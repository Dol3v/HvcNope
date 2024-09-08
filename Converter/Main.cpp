
// Declares clang::SyntaxOnlyAction.
#include "pch.h"    
#include <iostream>
#include "KernelFunctionConsumer.h"
#include "ReplaceIntrinisicsConsumer.h"
#include "ToolConfiguration.h"

static llvm::cl::list<std::string> KernelDirectories(
	"K",
	llvm::cl::desc( "Libraries that are assumed to contain kernel headers/definitions" ),
	llvm::cl::value_desc( "directory" )
);

static llvm::cl::OptionCategory MyToolCategory( "my-tool options" );

class MainConsumer : public ASTConsumer {
private:
	std::unique_ptr<KernelFunctionConsumer> kernelFunctionConsumer;
	std::unique_ptr<ReplaceIntrinsicsConsumer> intrinsicsConsumer;

	Rewriter& rewriter;
	bool includeHeader;

public:
	explicit MainConsumer( CompilerInstance* CI, Rewriter& R ) : rewriter( R ), includeHeader( false )
	{
		ASTContext* context = &CI->getASTContext();
		rewriter.setSourceMgr( context->getSourceManager(), context->getLangOpts() );

		// initialize consumer(s)
		kernelFunctionConsumer = make_unique<KernelFunctionConsumer>( context, rewriter, includeHeader );
		intrinsicsConsumer = make_unique<ReplaceIntrinsicsConsumer>( context, rewriter, includeHeader );
	}

	virtual void HandleTranslationUnit( ASTContext& Context ) 
	{
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

	// parse config from command options
	std::vector<std::string> kernelDirs( KernelDirectories.begin(), KernelDirectories.end() );
	ToolConfiguration::Initialize( kernelDirs );
	
	int result = Tool.run( newFrontendActionFactory<HvcnopeFrontendAction>().get() );
	return result;
}