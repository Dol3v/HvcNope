#pragma once

#include "pch.h"
#include <filesystem>

namespace Utils {

	// Clang related utilities
	namespace Clang 
	{
		//
		// Dumps the arguments of a CallExpr into a string, with a given delimeter between
		// the arguments.
		// 
		// For instance, the return value for the call
		//	int a = func(0, 3, nullptr, "a");
		// with delimeter "; " is 
		//	0; 3; nullptr; "a"
		//
		std::string DumpCallArguments(
			CallExpr* Call,
			const PrintingPolicy& PrintingPolicy,
			bool AddDelimeterAtStart = false,
			std::string_view Delimiter = ", " );

		bool HasAnnotateAttrWithName(
			const Decl* Declaration,
			const std::string& Name
		);
	}


	// Filesystem utlities
	namespace Fs 
	{
		namespace fs = std::filesystem;

		bool IsChildFolder( const fs::path& Parent, const fs::path& Child );
	}
}