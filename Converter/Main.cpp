
// Declares clang::SyntaxOnlyAction.
#include "pch.h"    
#include <iostream>
#include "KernelFunctionConsumer.h"
#include "ReplaceIntrinisicsConsumer.h"


llvm::cl::OptionCategory MyToolCategory( "my-tool options" );


DeclarationMatcher PointerWithAnnotationAttributeMatcher =
varDecl(
	hasType( pointerType() ),
	hasAttr( clang::attr::Annotate )
).bind( "ptrWithKernel" );

//StatementMatcher PossibleKernelPointerUsageMatcher = 
//    declRefExpr(to())

class PointerWithKernelAttrPrinter : public MatchFinder::MatchCallback {
public:
	virtual void run( const MatchFinder::MatchResult& Result ) {
		if (const VarDecl* VD = Result.Nodes.getNodeAs<VarDecl>( "ptrWithKernel" )) {
			if (const AnnotateAttr* AA = VD->getAttr<AnnotateAttr>()) {
				VD->dump();  // Dumps AST information for the matched node.

				llvm::outs() << "Found a pointer declaration with 'kernel' attribute: "
					<< VD->getNameAsString() << "\n";

			}
		}
	}
};


class MainConsumer : public ASTConsumer {
private:
	std::unique_ptr<KernelFunctionConsumer> kernelFunctionConsumer;
	std::unique_ptr<ReplaceIntrinsicsConsumer> intrinsicsConsumer;

	MatchFinder Matcher;
	PointerWithKernelAttrPrinter printer;

	Rewriter& rewriter;
	bool includeHeader;

public:
	explicit MainConsumer( CompilerInstance* CI, Rewriter& R ) : rewriter( R ), includeHeader( false )
	{
		Matcher.addMatcher( PointerWithAnnotationAttributeMatcher, &printer );

		ASTContext* context = &CI->getASTContext();
		rewriter.setSourceMgr( context->getSourceManager(), context->getLangOpts() );

		// initialize consumer(s)
		kernelFunctionConsumer = make_unique<KernelFunctionConsumer>( context, rewriter, includeHeader );
		intrinsicsConsumer = make_unique<ReplaceIntrinsicsConsumer>( context, rewriter, includeHeader );
	}

	virtual void HandleTranslationUnit( ASTContext& Context ) {
		Matcher.matchAST( Context );

		// call all consumers
		kernelFunctionConsumer->HandleTranslationUnit( Context );
		intrinsicsConsumer->HandleTranslationUnit( Context );

		if (includeHeader) {
			// add library include
			SourceManager& SM = Context.getSourceManager();
			FileID currentFileId = SM.getMainFileID();

			SourceLocation fileStart = SM.getLocForStartOfFile( currentFileId );

			std::string include = "#include <Hvcnope.h>\n";
			rewriter.InsertText( fileStart, include, false );
		}
	}
};

class HvcnopeFrontendAction : public ASTFrontendAction {
public:
	void EndSourceFileAction() override {
		outs() << "END OF FILE ACTION:\n";
		rewriter.getEditBuffer( rewriter.getSourceMgr().getMainFileID() ).write( outs() );
	}

	virtual std::unique_ptr<ASTConsumer> CreateASTConsumer( CompilerInstance& CI, StringRef file ) {
		return  std::unique_ptr<ASTConsumer>( new MainConsumer( &CI, rewriter ) );
	}

private:
	Rewriter rewriter;
};


extern llvm::cl::OptionCategory MyToolCategory;

int main( int argc, const char** argv ) 
{
	auto op = CommonOptionsParser::create( argc, argv, MyToolCategory );
	if (op.takeError()) {
		errs() << "\nUnexpected arguments\n";
		return 1;
	}
	ClangTool Tool( op->getCompilations(), op->getSourcePathList() );
	int result = Tool.run( newFrontendActionFactory<HvcnopeFrontendAction>().get() );
	return result;
}