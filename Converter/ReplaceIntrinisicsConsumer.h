#pragma once

#include "pch.h"

class ReplaceIntrinsicsVisitor : public RecursiveASTVisitor<ReplaceIntrinsicsVisitor> {
private:
	Rewriter& R;
	ASTContext* Context;

	static const std::map<std::string, std::string> IntrinsicNamesToFunctions;

public:
	ReplaceIntrinsicsVisitor( ASTContext* Context, Rewriter& R );

	virtual bool VisitCallExpr( CallExpr* Call );
};

//
// Replaces calls to kernel instrinsic functions (__readmsr, __out etc) with calls to our 
// intrinsics.
//
class ReplaceIntrinsicsConsumer : public ASTConsumer {
public:
	ReplaceIntrinsicsConsumer( ASTContext* Context, Rewriter& R );

	void HandleTranslationUnit( ASTContext& Context ) override;

private:
	ReplaceIntrinsicsVisitor Visitor;
};
