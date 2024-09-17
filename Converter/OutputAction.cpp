#include "pch.h"
#include "OutputAction.h"
#include "ToolConfiguration.h"

#include <filesystem>

class OutputVisitor : public RecursiveASTVisitor<OutputVisitor> {
public:
	explicit OutputVisitor( ASTContext* Context, Rewriter& R ) : Context( Context ), R( R ) {}

	virtual bool VisitAnnotateAttr( AnnotateAttr* AA ) {
		// verify it's ours
		StringRef annotation = AA->getAnnotation();
		if (annotation != "kernel" && annotation != "kernel_gs") return true;

		// nuke it
		R.RemoveText( AA->getRange() );
	}

private:
	ASTContext* Context;
	Rewriter& R;
};

class OutputConsumer : public ASTConsumer {
public:
	explicit OutputConsumer( ASTContext* Context, Rewriter& R ) : Visitor( Context, R ) {}

	void HandleTranslationUnit( clang::ASTContext& Context ) override {
		Visitor.TraverseDecl( Context.getTranslationUnitDecl() );
	}

private:
	OutputVisitor Visitor;
};

namespace fs = std::filesystem;

fs::path GetOutputPathFromSource( const fs::path& SourceFile ) {
	auto& config = ToolConfiguration::Instance();

	const fs::path& outputPath = config.GetOutputDirectory();
	const fs::path& rootPath = config.GetRootDirectory();

	// get relative path from root to SourceFile
	auto sourceRelative = fs::relative( SourceFile, rootPath );

	auto newSource = outputPath / sourceRelative;

	// create missing directories if necessary
	fs::create_directories( newSource.parent_path() );

	return newSource;
}

void OutputAction::EndSourceFileAction()
{
	auto sourceFilePath = fs::absolute( m_SourceFile.str() );
	auto newSource = GetOutputPathFromSource(sourceFilePath);

	outs() << "Writing into " << newSource.string() << "\n";

	std::error_code ec;
	llvm::raw_fd_ostream outputStream( newSource.string(), ec );
	if (ec) {
		errs() << "Failed to open output stream to write to " << newSource.string() << ", ec=" << ec.message() << "\n";
		throw std::runtime_error( "Failed to open new source" );
	}

	m_Rewriter.getEditBuffer( m_Rewriter.getSourceMgr().getMainFileID() ).write( outputStream );
}

std::unique_ptr<ASTConsumer> OutputAction::CreateASTConsumer( CompilerInstance& CI, StringRef File )
{
	// set the current source file we're working on
	m_SourceFile = File;
	return std::make_unique<OutputConsumer>( &CI.getASTContext(), m_Rewriter );
}
