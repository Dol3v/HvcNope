#include "pch.h"
#include "ToolConfiguration.h"

void ToolConfiguration::Initialize(
	const std::vector<std::string>& KernelDirs,
	const std::string& OutputPath,
	const std::string& RootDir )
{
	static bool initialized = false;
	if (initialized) throw std::runtime_error( "Re-initializing ToolConfiguration" );

	ToolConfiguration& instance = Instance();
	instance.KernelDirectories = KernelDirs;

	fs::path output( OutputPath );
	fs::path root( RootDir );

	if (!fs::exists( output ) || !fs::exists( root )) {
		errs() << "Output/root does not exist\n";
		throw std::runtime_error( "Invalid output/root" );
	}

	if (!fs::is_directory( OutputPath ) || !fs::is_directory(root)) {
		errs() << "Output/root paths are not directories\n";
		throw std::runtime_error( "Invalid output/root" );
	}

	instance.OutputPath = fs::absolute(output);
	instance.RootDirectory = fs::absolute(root);

	initialized = true;
}

bool ToolConfiguration::IsKernelSource( const fs::path& SourcePath ) const
{
	for (auto kernelDirectory : KernelDirectories)
	{
		if (Utils::Fs::IsChildFolder( kernelDirectory, SourcePath ))
			return true;
	}

	return false;
}

const fs::path& ToolConfiguration::GetOutputDirectory() const
{
	return OutputPath;
}

const fs::path& ToolConfiguration::GetRootDirectory() const
{
	return RootDirectory;
}
