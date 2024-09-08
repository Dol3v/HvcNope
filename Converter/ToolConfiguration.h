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

	static void Initialize( const std::vector<std::string>& KernelDirs );

	bool IsKernelSource( const fs::path& SourcePath ) const;

	ToolConfiguration( const ToolConfiguration& ) = delete;
	void operator=( const ToolConfiguration& ) = delete;

private:
	ToolConfiguration() : KernelDirectories() {}

private:
	std::vector<std::string> KernelDirectories;
};