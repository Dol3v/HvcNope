
// Declares clang::SyntaxOnlyAction.
#include "pch.h"    

#include <iostream>
#include <fstream>

#include "KernelFunctionConsumer.h"
#include "ReplaceIntrinisicsConsumer.h"
#include "ToolConfiguration.h"

//
// Command line options
//

static cl::OptionCategory DefaultCategory( "Converter options" );

static cl::list<std::string> KernelDirectories(
	"kernel-dir",
	cl::desc( "Libraries that are assumed to contain kernel headers/definitions" ),
	cl::value_desc( "directory" ),
	cl::cat(DefaultCategory)
);

static cl::alias KernelDirectoriesAlias(
	"K", cl::aliasopt( KernelDirectories ), cl::cat( DefaultCategory )
);

static cl::opt<std::string> OutputPath(
	"output-path",
	cl::desc( "Directory path to output the rewritten source to" ),
	cl::value_desc( "directory" ),
	cl::cat( DefaultCategory )
);

static cl::alias OutputPathAlias(
	"O", cl::aliasopt( OutputPath ), cl::cat( DefaultCategory )
);

static cl::opt<std::string> RootDir(
	"root-dir",
	cl::desc( "Root directory of all sources in the project, default is CWD" ),
	cl::value_desc( "directory" ),
	cl::cat( DefaultCategory )
);


class MainConsumer : public ASTConsumer {
private:
	std::unique_ptr<KernelFunctionConsumer> kernelFunctionConsumer;
	std::unique_ptr<ReplaceIntrinsicsConsumer> intrinsicsConsumer;

	Rewriter& rewriter;
	bool includeHeader;

public:
	explicit MainConsumer( CompilerInstance* CI, Rewriter& R ) : rewriter( R ), includeHeader( false )
	{
		ASTContext* context = &CI->getASTContext();
		rewriter.setSourceMgr( context->getSourceManager(), context->getLangOpts() );

		// initialize consumer(s)
		kernelFunctionConsumer = make_unique<KernelFunctionConsumer>( context, rewriter, includeHeader );
		intrinsicsConsumer = make_unique<ReplaceIntrinsicsConsumer>( context, rewriter, includeHeader );
	}

	virtual void HandleTranslationUnit( ASTContext& Context ) 
	{
		// call all consumers
		kernelFunctionConsumer->HandleTranslationUnit( Context );
		intrinsicsConsumer->HandleTranslationUnit( Context );

		if (includeHeader) {
			// add library include
			SourceManager& SM = Context.getSourceManager();
			FileID currentFileId = SM.getMainFileID();

			SourceLocation fileStart = SM.getLocForStartOfFile( currentFileId );

			std::string include = "#include <Hvcnope.h>\n";
			rewriter.InsertText( fileStart, include, false );
		}
	}
};

class HvcnopeFrontendAction : public ASTFrontendAction {
public:

	void EndSourceFileAction() override {
		outs() << "END OF FILE ACTION:\n";

		auto newSource = GetNewSourceFile();
		
		outs() << "Writing into " << newSource.string() << "\n";

		std::error_code ec;
		llvm::raw_fd_ostream outputStream( newSource.string(), ec );
		if (ec) {
			errs() << "Failed to open output stream to write to " << newSource.string() << ", ec=" << ec.message() << "\n";
			throw std::runtime_error( "Failed to open new source" );
		}

		rewriter.getEditBuffer( rewriter.getSourceMgr().getMainFileID() ).write( outputStream );
	}

	virtual std::unique_ptr<ASTConsumer> CreateASTConsumer( CompilerInstance& CI, StringRef file ) {
		currentFile = fs::absolute(file.str());
		return std::unique_ptr<ASTConsumer>( new MainConsumer( &CI, rewriter ) );
	}

private:

	fs::path GetNewSourceFile() {
		auto& config = ToolConfiguration::Instance();

		const fs::path& outputPath = config.GetOutputDirectory();
		const fs::path& rootPath = config.GetRootDirectory();

		// get relative path from root to currentFile
		auto sourceRelative = fs::relative( currentFile, rootPath );

		auto newSource = outputPath / sourceRelative;

		// create missing directories if necessary
		fs::create_directories( newSource.parent_path() );

		return newSource;
	}

private:
	Rewriter rewriter;
	fs::path currentFile;
};

extern llvm::cl::OptionCategory DefaultCategory;

int main( int argc, const char** argv ) 
{
	auto op = CommonOptionsParser::create( argc, argv, DefaultCategory );
	if (op.takeError()) {
		errs() << "\nUnexpected arguments\n";
		return 1;
	}
	ClangTool Tool( op->getCompilations(), op->getSourcePathList() );

	// parse config from command options
	std::vector<std::string> kernelDirs( KernelDirectories.begin(), KernelDirectories.end() );
	ToolConfiguration::Initialize( kernelDirs, OutputPath, RootDir );
	
	int result = Tool.run( newFrontendActionFactory<HvcnopeFrontendAction>().get() );
	return result;
}
