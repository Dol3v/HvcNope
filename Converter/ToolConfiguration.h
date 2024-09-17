#pragma once
#include <vector>
#include <string>

#include "Utils.h"

namespace fs = std::filesystem;

//
// Tool configuration options
//
class ToolConfiguration {
public:

	static ToolConfiguration& Instance() {
		static ToolConfiguration instance;
		return instance;
	}

	static void Initialize( const std::vector<std::string>& KernelDirs, const std::string& OutputPath, const std::string& RootDir );

	bool IsKernelSource( const fs::path& SourcePath ) const;

	const fs::path& GetOutputDirectory() const;
	const fs::path& GetRootDirectory() const;

	Rewriter& GetRewriter() { return TheRewriter; }

	ToolConfiguration( const ToolConfiguration& ) = delete;
	void operator=( const ToolConfiguration& ) = delete;

private:
	ToolConfiguration() : KernelDirectories() {}

private:
	std::vector<std::string> KernelDirectories;
	fs::path OutputPath;
	fs::path RootDirectory;

	Rewriter TheRewriter;
};