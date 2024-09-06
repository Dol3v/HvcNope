
#include "pch.h"
#include "KernelFunctionConsumer.h"

KernelFunctionConsumer::KernelFunctionConsumer( ASTContext* Context, Rewriter& Rw ) :
	Finder(Context, KernelFunctions),
	R( Rw )
{
}

void KernelFunctionConsumer::HandleTranslationUnit( ASTContext& Context )
{
	// Collect all new kernel functions from the new translation unit
	Finder.TraverseDecl( Context.getTranslationUnitDecl() );

	// Rewrite all function calls
	KernelCallRewriter callRewriter( &Context, R, KernelFunctions );
	callRewriter.TraverseDecl( Context.getTranslationUnitDecl() );


	// Log rewritten stuff to file
	auto& SM = Context.getSourceManager();
	auto currentFileId = SM.getMainFileID();

	outs() << "Rewrote " << SM.getFileEntryForID( currentFileId )->tryGetRealPathName() << "\n";

	R.getRewriteBufferFor( currentFileId )->write(outs());
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
	SourceManager& SM = Context->getSourceManager();

	auto currentFilename = SM.getFilename( Call->getBeginLoc() ).str();
	outs() << "Looking at " << currentFilename << "\n";

	if (FunctionDecl* FD = Call->getDirectCallee()) {
		SourceLocation loc = FD->getLocation();
		if (loc.isValid()) {
			auto declarationFilePath = SM.getFilename( loc ).str();
			if (IsFileInWdkKm( declarationFilePath )) {
				// kernel function, add
				KernelFunctions.insert( FD->getNameAsString() );
				outs() << "Function " << FD->getNameAsString() << " is kernel, defined in headers\n";
			}
		}
		else {
			errs() << "Call to function " << FD->getNameAsString() << " has invalid location\n";
		}
	}
	return true;
}

bool KernelFunctionFinder::IsFileInWdkKm( const std::string_view Filename )
{
	return true;
}

KernelCallRewriter::KernelCallRewriter( ASTContext* Context, Rewriter& R, const std::set<std::string>& Functions )
	: Context(Context), R(R), KernelFunctions(Functions)
{
}

bool KernelCallRewriter::VisitCallExpr( CallExpr* Call )
{
	if (FunctionDecl* FD = Call->getDirectCallee()) {
		std::string functionName = FD->getNameInfo().getName().getAsString();

		// Check if the function is in our target list
		if (KernelFunctions.find( functionName ) != KernelFunctions.end()) {
			SourceManager& SM = Context->getSourceManager();
			SourceLocation startLoc = Call->getBeginLoc();

			// Generate new call expression
			std::string newCall = "g_Invoker.Call(\"" + functionName + "\"";

			// Add each argument
			for (unsigned i = 0; i < Call->getNumArgs(); ++i) {
				Expr* arg = Call->getArg( i );
				newCall += ", ";
				std::string ArgStr;
				llvm::raw_string_ostream ArgStream( ArgStr );
				arg->printPretty( ArgStream, nullptr, Context->getPrintingPolicy() );
				newCall += ArgStream.str();
			}

			newCall += ")";

			// Replace the original function call with the new one
			R.ReplaceText( Call->getSourceRange(), newCall );
		}
	}
	return true;
}
