#pragma once

#include "pch.h"
#include <filesystem>

namespace Utils {

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

	std::optional<const FunctionDecl*> GetContainingFunctionDecl(
		const CallExpr* Call,
		ASTContext* Context
	);

	namespace fs = std::filesystem;

	bool IsChildFolder( const fs::path& Parent, const fs::path& Child );
}