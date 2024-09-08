#pragma once

#include "pch.h"

class ReplaceIntrinsicsVisitor : public RecursiveASTVisitor<ReplaceIntrinsicsVisitor> {
private:
	Rewriter& R;
	ASTContext* Context;
	std::set<std::string> KernelGsFunctionNames;
	

	// Names of intrinsics functions mapped to wrapper implementation names
	static const std::map<std::string, std::string> IntrinsicNamesToFunctions;
	
	//
	// Names of GS-related intrinsics functions mapped to their wrapper implementation names.
	// 
	// Gs-related intrinsics are handled separately as we only want to replace the ones expecting
	// to find the kernel GS register (KeGetCurrentThread, for instance) but not the ones expecting
	// the usermode GS (anything PEB/TEB related)
	//
	static const std::map<std::string, std::string> GsIntrinsicNamesToFunctions;

public:
	ReplaceIntrinsicsVisitor( ASTContext* Context, Rewriter& R );

	//
	// Replaces calls to intrisics with their wrappers.
	//
	virtual bool VisitCallExpr( CallExpr* Call );

	//
	// Finds all functions with the KERNEL_GS attribute.
	//
	virtual bool VisitFunctionDecl( FunctionDecl* FD );

private:
	static std::optional<std::string> GetIntrinsicWrapperNameIfNecessary( CallExpr* Call );
};

//
// Replaces calls to kernel instrinsic functions (__readmsr, __outbyte etc) with calls to our 
// intrinsics.
//
class ReplaceIntrinsicsConsumer : public ASTConsumer {
public:
	ReplaceIntrinsicsConsumer( ASTContext* Context, Rewriter& R );

	void HandleTranslationUnit( ASTContext& Context ) override;

private:
	ReplaceIntrinsicsVisitor Visitor;
};
