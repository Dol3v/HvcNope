
#include "pch.h"
#include "KernelFunctionConsumer.h"

KernelFunctionConsumer::KernelFunctionConsumer( ASTContext* Context, Rewriter& R ) :
	Finder(Context, KernelFunctions),
	R(Context->getSourceManager(), Context->getLangOpts())
{
}

void KernelFunctionConsumer::HandleTranslationUnit( ASTContext& Context )
{
	// Collect all new kernel functions from the new translation unit
	Finder.TraverseDecl( Context.getTranslationUnitDecl() );

	// Rewrite all function calls
	/*KernelCallRewriter callRewriter( &Context, R, KernelFunctions );
	callRewriter.TraverseDecl( Context.getTranslationUnitDecl() );*/


	// Log rewritten stuff to file
	//auto& SM = Context.getSourceManager();
	//auto currentFileId = SM.getMainFileID();

	//outs() << "Rewrote " << SM.getFileEntryForID( currentFileId )->tryGetRealPathName() << "\n";

	//R.getRewriteBufferFor( currentFileId )->write(outs());
}

KernelFunctionFinder::KernelFunctionFinder( ASTContext* Context, std::set<std::string>& Functions ) :
	Context(Context), KernelFunctions( Functions )
{
}

bool KernelFunctionFinder::VisitFunctionDecl( FunctionDecl* FD )
{
	// Check if the function decleration has our kernel annotate attribute
	if (const AnnotateAttr* attr = FD->getAttr<AnnotateAttr>()) {
		if (attr->getAnnotation() == "kernel") {
			auto functionName = FD->getNameAsString();

			outs() << "Function " << functionName << " has a kernel annotation attribute, hence kernel function\n";
			KernelFunctions.insert( functionName );
		}
	}

	return true;
}

bool KernelFunctionFinder::VisitCallExpr( CallExpr* Call )
{
	if (FunctionDecl* FD = Call->getDirectCallee()) {
		SourceLocation loc = FD->getLocation();
		if (loc.isValid()) {
			SourceManager& SM = Context->getSourceManager();
			auto filename = SM.getFilename( loc ).str();

			outs() << "Call to function " << FD->getNameAsString() << " has valid location, filename " << filename << "\n";
		}
		else {
			errs() << "Call to function " << FD->getNameAsString() << " has invalid location\n";
		}
	}
	return true;
}

//bool KernelFunctionFinder::IsFileInWdkKm( const std::string_view Filename )
//{
//	return true;
//}
//
//KernelCallRewriter::KernelCallRewriter( ASTContext* Context, Rewriter& R, const std::set<std::string>& KernelFunctions )
//{
//}
//
//bool KernelCallRewriter::VisitCallExpr( CallExpr* Call )
//{
//	return true;
//}
