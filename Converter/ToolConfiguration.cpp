#include "pch.h"
#include "ToolConfiguration.h"

void ToolConfiguration::Initialize( const std::vector<std::string>& KernelDirs )
{
    static bool initialized = false;
    if (initialized) throw std::runtime_error( "Re-initializing ToolConfiguration" );

    ToolConfiguration& instance = Instance();
    instance.KernelDirectories = KernelDirs;

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
