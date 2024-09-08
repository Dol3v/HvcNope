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
	// Mark all called functions that come from the Windows WDK km folder as 
	// kernel functions.
	//
	virtual bool VisitCallExpr( CallExpr* Call );

private:

	bool IsFileInWdkKm( const std::string_view Filename );

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
	std::set<std::string> KernelFunctions;
	KernelFunctionFinder Finder;
	Rewriter R;
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