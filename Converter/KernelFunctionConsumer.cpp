
#include "pch.h"
#include "KernelFunctionConsumer.h"
#include "Utils.h"
#include "ToolConfiguration.h"

KernelFunctionConsumer::KernelFunctionConsumer( ASTContext* Context, Rewriter& Rewriter, bool& IncludeLibraryHeader ) :
	Finder( Context, KernelFunctions, IncludeLibraryHeader ), R( Rewriter )
{
	//FunctionPointerMatcher.addMatcher(
	//	IsAnnotatedFunctionPointer.bind( "kernelFunctionPointer" ),
	//	CreateKernelFunctionPtrHandler().get()
	//);

	//FunctionPointerMatcher.addMatcher(
	//	callExpr( callee( declRefExpr( to( IsAnnotatedFunctionPointer ) ) ) ).bind( "kernelFunctionPtrCall" ),
	//	CreateKernelFunctionPtrCallHandler().get()
	//);
}

auto IsAnnotatedFunctionPointer =
varDecl( hasType( pointerType( pointee( functionType() ) ) ),
	hasAttr( clang::attr::AnnotateType ) );

void KernelFunctionConsumer::HandleTranslationUnit( ASTContext& Context )
{
	// Collect all new kernel functions from declarations/attributes
	// from the new translation unit
	Finder.TraverseDecl( Context.getTranslationUnitDecl() );

	// Add all new kernel function pointers
	//FunctionPointerMatcher.matchAST( Context );

	// Rewrite all function calls
	KernelCallRewriter callRewriter( &Context, R, KernelFunctions );
	callRewriter.TraverseDecl( Context.getTranslationUnitDecl() );


	// Log rewritten stuff to file
	auto& SM = Context.getSourceManager();
	auto currentFileId = SM.getMainFileID();

	outs() << "Rewrote " << SM.getFileEntryForID( currentFileId )->tryGetRealPathName() << "\n";
}

//std::unique_ptr<MatchFinder::MatchCallback> KernelFunctionConsumer::CreateKernelFunctionPtrHandler()
//{
//	return std::make_unique<MatchFinder::MatchCallback>( 
//		[&]( const MatchFinder::MatchResult& Result ) {
//			if (const VarDecl* var = Result.Nodes.getNodeAs<VarDecl>( "kernelFunctionPointer" )) {
//				if (const AnnotateAttr* attr = var->getAttr<AnnotateAttr>()) {
//					if (attr->getAnnotation() == "kernel") {
//
//					}
//				}
//			}
//		});
//}

//std::unique_ptr<MatchFinder::MatchCallback> KernelFunctionConsumer::CreateKernelFunctionPtrCallHandler()
//{
//	return std::make_unique<MatchFinder::MatchCallback>(
//		[&]( const MatchFinder::MatchResult& Result ) {
//			const auto* callExpr = Result.Nodes.getNodeAs<CallExpr>( "kernelFunctionPtrCall" );
//			if (!callExpr) return;
//
//			const FunctionDecl* calleeDecl = callExpr->getDirectCallee();
//			if (!calleeDecl) return;
//
//			for (auto attr : calleeDecl->getAttrs()) {
//				if (const auto* annotate = dyn_cast<AnnotateAttr>(attr)) {
//					if (annotate->getAnnotation() == "kernel") {
//						KernelFunctions.insert()
//					}
//				}
//			}
//		} );
//}

KernelFunctionFinder::KernelFunctionFinder( ASTContext* Context, std::set<std::string>& Functions, bool& IncludeLibraryHeader ) :
	Context( Context ), KernelFunctions( Functions ), IncludeLibraryHeader( IncludeLibraryHeader ) {}

bool KernelFunctionFinder::VisitFunctionDecl( FunctionDecl* FD )
{
	// Check if the function decleration has our kernel annotate attribute
	if (Utils::Clang::HasAnnotateAttrWithName( FD, "kernel" )) {
		auto functionName = FD->getNameAsString();
		KernelFunctions.insert( functionName );
	}

	return true;
}

bool KernelFunctionFinder::VisitCallExpr( CallExpr* Call )
{
	SourceManager& SM = Context->getSourceManager();

	auto currentFilename = SM.getFilename( Call->getBeginLoc() ).str();
	outs() << "Looking at " << currentFilename << "\n";

	// handle calls to functions from kernel sources
	if (FunctionDecl* FD = Call->getDirectCallee()) {
		SourceLocation loc = FD->getLocation();
		if (loc.isValid()) {
			auto declarationFilePath = SM.getFilename( loc ).str();
			if (ToolConfiguration::Instance().IsKernelSource( declarationFilePath )) {
				// kernel function, add
				KernelFunctions.insert( FD->getNameAsString() );
				outs() << "Function " << FD->getNameAsString() << " is kernel, defined in headers\n";


				// we've found a call to a kernel function, should include header for call
				IncludeLibraryHeader = true;
			}
		}
		else {
			errs() << "Call to function " << FD->getNameAsString() << " has invalid location\n";
		}
	}
	return true;
}

KernelCallRewriter::KernelCallRewriter( ASTContext* Context, Rewriter& R, const std::set<std::string>& Functions )
	: Context( Context ), R( R ), KernelFunctions( Functions )
{
}

bool KernelCallRewriter::VisitCallExpr( CallExpr* Call )
{
	bool rewriteCall = false;
	std::string calleeExpr;

	// handle calls to kernel function pointers
	if (const VarDecl* VD = dyn_cast<VarDecl>(Call->getCalleeDecl())) {
		// if it's a function pointer that has the kernel attribute
		if (VD->getType()->isPointerType() &&
			VD->getType()->getPointeeType()->isFunctionType() &&
			Utils::Clang::HasAnnotateAttrWithName( VD, "kernel" ))
		{
			rewriteCall = true;

			//
			// We call the kernel function pointer, assuming it points to a kernel address.
			// Hence we'll add a C-style cast to kAddress.
			//
			calleeExpr = "kAddress(" + VD->getNameAsString() + ")";
		}
	}
	else if (FunctionDecl* FD = Call->getDirectCallee()) {
		std::string functionName = FD->getNameInfo().getName().getAsString();

		// Check if the function is in our target list
		if (KernelFunctions.find( functionName ) != KernelFunctions.end()) 
		{
			rewriteCall = true;

			//
			// We call the kernel function by name, assuming it's exported -
			// this we use the CallKernelFunction overload that accepts the function name as a string.
			//
			calleeExpr = "\"" + functionName + "\"";
		}
	}

	if (rewriteCall) {
		// Rewrite return type - g_Invoker.CallKernelFunction returns a generic Qword type,
		// hence to avoid compilation errors we'll need to cast the return type to the expected
		// one
		QualType returnType = Call->getCallReturnType( *Context );
		std::string newCall = "(" + returnType.getAsString() + ") g_Invoker->CallKernelFunction(" + calleeExpr;

		newCall += Utils::Clang::DumpCallArguments(
			Call,
			Context->getPrintingPolicy(),
			/*AddDelimeterAtStart=*/true
		);

		newCall += ")";

		// Replace the original function call with the new one
		R.ReplaceText( Call->getSourceRange(), newCall );
	}

	return true;
}
