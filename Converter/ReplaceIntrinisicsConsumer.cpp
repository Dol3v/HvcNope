#include "pch.h"
#include "ReplaceIntrinisicsConsumer.h"
#include "Utils.h"

ReplaceIntrinsicsConsumer::ReplaceIntrinsicsConsumer( ASTContext* Context, Rewriter& R, bool& IncludeLibraryHeader )
 : Visitor( Context, R, IncludeLibraryHeader ) {}

void ReplaceIntrinsicsConsumer::HandleTranslationUnit( ASTContext& Context )
{
	Visitor.TraverseTranslationUnitDecl( Context.getTranslationUnitDecl() );
}

ReplaceIntrinsicsVisitor::ReplaceIntrinsicsVisitor( ASTContext* Context, Rewriter& R, bool& IncludeLibraryHeader )
	: Context(Context), R(R), IncludeLibraryHeader( IncludeLibraryHeader ) {}

const std::map<std::string, std::string> ReplaceIntrinsicsVisitor::IntrinsicNamesToFunctions = 
{
	// MSR-related instrinsics
	{"__readmsr", "Hvcnope_readmsr"},
	{"__writemsr", "Hvcnope_writemsr"},

	// In/Out intrinsics
	{"__inbyte", "Hvcnope_inbyte"},
	{"__inbytestring", "Hvcnope_inbytestring"},
	{"__inword", "Hvcnope_inword"},
	{"__inwordstring", "Hvcnope_inwordstring"},
	{"__indword", "Hvcnope_indword"},
	{"__indwordstring", "Hvcnope_indwordstring"},

	{"__outbyte", "Hvcnope_outbyte"},
	{"__outbytestring", "Hvcnope_outbytestring"},
	{"__outword", "Hvcnope_outword"},
	{"__outwordstring", "Hvcnope_outwordstring"},
	{"__outdword", "Hvcnope_outdword"},
	{"__outdwordstring", "Hvcnope_outdwordstring"},
};

const std::map<std::string, std::string> ReplaceIntrinsicsVisitor::GsIntrinsicNamesToFunctions =
{
	{"__readgsbyte", "Hvcnope__readgsbyte"},
	{"__readgsword", "Hvcnope__readgsword"},
	{"__readgsdword", "Hvcnope__readgsdword"},
	{"__readgsqword", "Hvcnope__readgsqword"},

	{"__writegsbyte", "Hvcnope__writegsbyte"},
	{"__writegsword", "Hvcnope__writegsword"},
	{"__writegsdword", "Hvcnope__writegsdword"},
	{"__writegsqword", "Hvcnope__writegsqword"},

	{"__incgsbyte", "Hvcnope__incgsbyte"},
	{"__incgsword", "Hvcnope__incgsword"},
	{"__incgsdword", "Hvcnope__incgsdword"},
	{"__incgsqword", "Hvcnope__incgsqword"},

	{"__addgsbyte", "Hvcnope__addgsbyte"},
	{"__addgsword", "Hvcnope__addgsword"},
	{"__addgsdword", "Hvcnope__addgsdword"},
	{"__addgsqword", "Hvcnope__addgsqword"}
};

std::optional<std::string> ReplaceIntrinsicsVisitor::GetIntrinsicWrapperNameIfNecessary( CallExpr* Call )
{
	if (const FunctionDecl* FD = Call->getDirectCallee()) {
		std::string calleeName = FD->getNameAsString();
		if (IntrinsicNamesToFunctions.find( calleeName ) != IntrinsicNamesToFunctions.end()) {
			// callee is an intrinsic we always replace
			return IntrinsicNamesToFunctions.at( calleeName );
		}

		if (GsIntrinsicNamesToFunctions.find( calleeName ) != GsIntrinsicNamesToFunctions.end()) 
		{
			// check if CallExpr is inside known GS source range
			for ( SourceRange gsRange : KernelGsSourceRanges )
			{
				if (gsRange.fullyContains( Call->getSourceRange() )) {
					return GsIntrinsicNamesToFunctions.at( calleeName );
				}
			}
		}

		return std::nullopt;
	}
}

bool ReplaceIntrinsicsVisitor::VisitCallExpr( CallExpr* Call )
{	
	if (const FunctionDecl* FD = Call->getDirectCallee()) {
		auto wrapperName = GetIntrinsicWrapperNameIfNecessary( Call );
		if (wrapperName) {
			// replace call
			std::string newCall = wrapperName.value() + "(";
			newCall += Utils::DumpCallArguments( Call, Context->getPrintingPolicy() );
			newCall += ")";

			R.ReplaceText( Call->getSourceRange(), newCall );
		
			// Calling into library implementation, should include header
			IncludeLibraryHeader = true;
		}
	}
	return true;
}

bool ReplaceIntrinsicsVisitor::VisitFunctionDecl( FunctionDecl* FD )
{
	if (AnnotateAttr* attr = FD->getAttr<AnnotateAttr>()) {
		if (attr->getAnnotation() == "kernel_gs") {
			//KernelGsFunctionNames.insert( FD->getNameAsString() );
			FD->getSourceRange().dump(Context->getSourceManager());

			// TODO: handle headers, use sorted vec
			KernelGsSourceRanges.push_back( FD->getSourceRange() );
		}
	}

	return true;
}
