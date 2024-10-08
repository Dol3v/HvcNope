#include "pch.h"
#include "Utils.h"
#include <iostream>

std::string Utils::Clang::DumpCallArguments( 
	CallExpr* Call,
	const PrintingPolicy& PrintingPolicy,
	bool AddDelimeterAtStart, 
	std::string_view Delimiter )
{
    std::string result;

	for (unsigned int i = 0; i < Call->getNumArgs(); ++i) {
		Expr* arg = Call->getArg( i );

		std::string argString;
		llvm::raw_string_ostream argStream( argString );
		arg->printPretty( argStream, nullptr, PrintingPolicy );
		
		// Add delimeter if we're not at the first argument,
		// or if we're supposed to always add a delimeter, even at the start
		//
		if (i != 0 || AddDelimeterAtStart) {
			result += Delimiter;
		}
		result += argStream.str();
	}

    return result;
}

bool Utils::Clang::HasAnnotateAttrWithName( const Decl* Declaration, const std::string& Name )
{
	if (!Declaration->hasAttrs()) return false;

	for (const Attr* attr : Declaration->getAttrs()) {
		if (const auto* annotateAttr = dyn_cast<AnnotateAttr>(attr)) {
			if (annotateAttr->getAnnotation() == Name) {
				return true;
			}
		}
	}
	return false;
}

bool Utils::Fs::IsChildFolder( const fs::path& Parent, const fs::path& Child ) {

	// Check if both paths are directories and parent is indeed a parent of child
	if (fs::is_directory( Parent )) {
		auto parentCanonical = fs::canonical( Parent );
		auto childCanonical = fs::canonical( Child );

		return std::mismatch( parentCanonical.begin(), parentCanonical.end(), childCanonical.begin() ).first == parentCanonical.end();
	}
	return false;
}
