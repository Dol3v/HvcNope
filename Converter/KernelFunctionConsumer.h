#pragma once

#include "pch.h"

//
// Rewrites all calls to kernel functions with calls to our kernel function
// invoker.
//

//
// Marks all kernel functions in a translation unit.
//
class KernelFunctionFinder : public RecursiveASTVisitor<KernelFunctionFinder> {
public:

	KernelFunctionFinder( ASTContext* Context, std::set<std::string>& KernelFunctions, bool& IncludeLibraryHeader );

	//
	// Mark all declared functions with a kernel attribute.
	//
	virtual bool VisitFunctionDecl( FunctionDecl* FD );

	//
	// Mark all called functions that come from a kernel-marked source file/kernel function pointers as
	// kernel functions.
	// 
	// See the kernel directories option of our tool.
	//
	virtual bool VisitCallExpr( CallExpr* Call );


private:
	ASTContext* Context;
	std::set<std::string>& KernelFunctions;
	bool& IncludeLibraryHeader;
};

//
// AST Consumer that finds and re-writes all kernel function calls.
//
class KernelFunctionConsumer : public ASTConsumer {
public:
	KernelFunctionConsumer( ASTContext* Context, Rewriter& R, bool& IncludeLibraryHeader );

	void HandleTranslationUnit( ASTContext& Context ) override;

private:
	
	//std::unique_ptr<MatchFinder::MatchCallback> CreateKernelFunctionPtrHandler();

	//std::unique_ptr<MatchFinder::MatchCallback> CreateKernelFunctionPtrCallHandler();

private:
	std::set<std::string> KernelFunctions;
	KernelFunctionFinder Finder;
	Rewriter& R;
	//MatchFinder FunctionPointerMatcher;
};

class KernelCallRewriter : public RecursiveASTVisitor<KernelCallRewriter> {
public:
	KernelCallRewriter( ASTContext* Context, Rewriter& R, const std::set<std::string>& KernelFunctions );

	virtual bool VisitCallExpr( CallExpr* Call );

private:
	ASTContext* Context;
	Rewriter& R;
	const std::set<std::string>& KernelFunctions;
};