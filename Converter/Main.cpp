
// Declares clang::SyntaxOnlyAction.
#include "pch.h"    
#include <iostream>


Rewriter rewriter;
int numFunctions = 0;
llvm::cl::OptionCategory MyToolCategory( "my-tool options" );

class ExampleVisitor : public RecursiveASTVisitor<ExampleVisitor> {
private:
    ASTContext* astContext; // used for getting additional AST info

public:
    explicit ExampleVisitor( CompilerInstance* CI )
        : astContext( &(CI->getASTContext()) ) // initialize private members
    {
        rewriter.setSourceMgr( astContext->getSourceManager(),
            astContext->getLangOpts() );
    }

    virtual bool VisitAttr(Attr* attr) {
        outs() << "Found attribute!\n";
        attr->getLocation().dump( astContext->getSourceManager() );
        return true;
    }
    

    virtual bool VisitFunctionDecl( FunctionDecl* func ) {
        numFunctions++;
        string funcName = func->getNameInfo().getName().getAsString();
        if (funcName == "do_math") {
            rewriter.ReplaceText( func->getLocation(), funcName.length(), "add5" );
            errs() << "** Rewrote function def: " << funcName << "\n";
        }
        return true;
    }

    virtual bool VisitStmt( Stmt* st ) {
        if (ReturnStmt* ret = dyn_cast<ReturnStmt>(st)) {
            rewriter.ReplaceText( ret->getRetValue()->getBeginLoc(), 6, "val" );
            errs() << "** Rewrote ReturnStmt\n";
        }
        if (CallExpr* call = dyn_cast<CallExpr>(st)) {
            rewriter.ReplaceText( call->getBeginLoc(), 7, "add5" );
            errs() << "** Rewrote function call\n";
        }
        return true;
    }
};


auto pointerWithKernelAttrMatcher =
varDecl(
    hasType( pointerType() ),
    hasAttr( clang::attr::Annotate ) // GCC/Clang-style __attribute__((annotate("kernel")))
).bind( "ptrWithKernel" );

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


class ExampleASTConsumer : public ASTConsumer {
private:
    ExampleVisitor* visitor; // doesn't have to be private
    MatchFinder matcher;
    PointerWithKernelAttrPrinter printer;

public:
    // override the constructor in order to pass CI
    explicit ExampleASTConsumer( CompilerInstance* CI )
        : visitor( new ExampleVisitor( CI ) ) // initialize the visitor
    {
        matcher.addMatcher( pointerWithKernelAttrMatcher, &printer);
    }

    // override this to call our ExampleVisitor on the entire source file
    virtual void HandleTranslationUnit( ASTContext& Context ) {
        /* we can use ASTContext to get the TranslationUnitDecl, which is
             a single Decl that collectively represents the entire source file */
        //visitor->TraverseDecl( Context.getTranslationUnitDecl() );
        matcher.matchAST( Context );
    }

};



class ExampleFrontendAction : public ASTFrontendAction {
public:

    void EndSourceFileAction() override {
        llvm::outs() << "END OF FILE ACTION:\n";
        rewriter.getEditBuffer( rewriter.getSourceMgr().getMainFileID() ).write( errs() );
    }

    virtual std::unique_ptr<ASTConsumer> CreateASTConsumer( CompilerInstance& CI, StringRef file ) {
        return  std::unique_ptr<ASTConsumer>( new ExampleASTConsumer( &CI ) ); // pass CI pointer to ASTConsumer
    }
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

    int result = Tool.run( newFrontendActionFactory<ExampleFrontendAction>().get() );
    return result;
}