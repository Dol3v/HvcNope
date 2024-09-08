#include "pch.h"
#include "Utils.h"
#include <iostream>

std::string Utils::DumpCallArguments( 
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

std::optional<const FunctionDecl*> Utils::GetContainingFunctionDecl( const CallExpr* Call, ASTContext* Context )
{
	outs() << "Finding out containing function\n";
	Call->dump();

    const auto parents = Context->getParents( *Call );
    if (parents.empty()) {
        return std::nullopt;
    }

    const DynTypedNode& parentNode = parents[0];

    // Traverse upwards through the parent nodes
    const Decl* currentDecl = nullptr;
    while (true) {
        if (auto* parentDecl = parentNode.get<Decl>()) {
            currentDecl = parentDecl;

            // Check if we hit a FunctionDecl
            if (auto* funcDecl = dyn_cast<FunctionDecl>(currentDecl)) {
                return funcDecl;
            }
        }

        // Get the next parent
        const auto nextParents = Context->getParents( *currentDecl );
        if (nextParents.empty()) {
            break;
        }
        currentDecl = nextParents[0].get<Decl>();
    }

    return std::nullopt;
}

bool Utils::IsChildFolder( const fs::path& Parent, const fs::path& Child ) {

	// Check if both paths are directories and parent is indeed a parent of child
	if (fs::is_directory( Parent )) {
		auto parentCanonical = fs::canonical( Parent );
		auto childCanonical = fs::canonical( Child );

		return std::mismatch( parentCanonical.begin(), parentCanonical.end(), childCanonical.begin() ).first == parentCanonical.end();
	}
	return false;
}
