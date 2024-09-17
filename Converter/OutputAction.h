#pragma once

#include "pch.h"

#include "ToolConfiguration.h"

//
// Responsible for cleaning-up and outputting the modified sources.
//
class OutputAction : public ASTFrontendAction {
public:

	OutputAction() : m_Rewriter( ToolConfiguration::Instance().GetRewriter() ) {}

	void EndSourceFileAction() override;

	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer( clang::CompilerInstance& CI, llvm::StringRef File ) override;

private:
	Rewriter& m_Rewriter;
	StringRef m_SourceFile;
};
