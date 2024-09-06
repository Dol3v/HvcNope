
// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

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


class ExampleASTConsumer : public ASTConsumer {
private:
    ExampleVisitor* visitor; // doesn't have to be private

public:
    // override the constructor in order to pass CI
    explicit ExampleASTConsumer( CompilerInstance* CI )
        : visitor( new ExampleVisitor( CI ) ) // initialize the visitor
    { }

    // override this to call our ExampleVisitor on the entire source file
    virtual void HandleTranslationUnit( ASTContext& Context ) {
        /* we can use ASTContext to get the TranslationUnitDecl, which is
             a single Decl that collectively represents the entire source file */
        visitor->TraverseDecl( Context.getTranslationUnitDecl() );
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

    // create a new Clang Tool instance (a LibTooling environment)
    ClangTool Tool( op->getCompilations(), op->getSourcePathList() );

    // run the Clang Tool, creating a new FrontendAction (explained below)
    int result = Tool.run( newFrontendActionFactory<ExampleFrontendAction>().get() );

    errs() << "\nFound " << numFunctions << " functions.\n\n";
    // print out the rewritten source code ("rewriter" is a global var.)

    std::cin.get();
    return result;
}