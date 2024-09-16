#pragma once

#include "pch.h"
#include <filesystem>

//
// For RecrusiveASTVisitor, skip an expression\statement if it is not present
// in the main file
//
#define VISITOR_SKIP_NON_MAIN_FILES(_x_, _Context_) \
	SourceManager& _SM_ = (_Context_)->getSourceManager(); \
	if (!_SM_.isInMainFile((_x_)->getBeginLoc())) return true;

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