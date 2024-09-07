
// Declares clang::SyntaxOnlyAction.
#include "pch.h"    
#include <iostream>
#include "KernelFunctionConsumer.h"
#include "ReplaceIntrinisicsConsumer.h"


llvm::cl::OptionCategory MyToolCategory( "my-tool options" );


DeclarationMatcher PointerWithAnnotationAttributeMatcher =
    varDecl(
        hasType( pointerType() ),
        hasAttr( clang::attr::Annotate)
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
            /*    else if (VD->hasAttr<MSDeclSpecAttr>()) {
                    llvm::outs() << "This is MSVC-style __declspec(kernel)\n";
                }*/
        }
    }
};


class MainConsumer : public ASTConsumer {
private:
    std::unique_ptr<KernelFunctionConsumer> kernelFunctionConsumer;
    std::unique_ptr<ReplaceIntrinsicsConsumer> intrinsicsConsumer;

    MatchFinder matcher;
    PointerWithKernelAttrPrinter printer;

    Rewriter& rewriter;

public:
    // override the constructor in order to pass CI
    explicit MainConsumer( CompilerInstance* CI, Rewriter& R ) : rewriter(R)
    {
        matcher.addMatcher( PointerWithAnnotationAttributeMatcher, &printer);

        ASTContext* context = &CI->getASTContext();
        rewriter.setSourceMgr( context->getSourceManager(), context->getLangOpts() );

        // initialize consumer(s)
        kernelFunctionConsumer = make_unique<KernelFunctionConsumer>( context, rewriter );
        intrinsicsConsumer = make_unique<ReplaceIntrinsicsConsumer>( context, rewriter );
    }

    // override this to call our ExampleVisitor on the entire source file
    virtual void HandleTranslationUnit( ASTContext& Context ) {
        matcher.matchAST( Context );

        // call all consumers
        kernelFunctionConsumer->HandleTranslationUnit( Context );
        intrinsicsConsumer->HandleTranslationUnit( Context );
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

int main( int argc, const char** argv ) {
    // parse the command-line args passed to your code
    //CommonOptionsParser op( argc, argv, MyToolCategory );
    auto op = CommonOptionsParser::create( argc, argv, MyToolCategory );
    if (op.takeError()) {
        errs() << "\nUnexpected arguments\n";
        return 1;
    }
    ClangTool Tool( op->getCompilations(), op->getSourcePathList() );
    int result = Tool.run( newFrontendActionFactory<HvcnopeFrontendAction>().get() );
    return result;
}