#include "pch.h"
#include "ReplaceIntrinisicsConsumer.h"
#include "Utils.h"

ReplaceIntrinsicsConsumer::ReplaceIntrinsicsConsumer( ASTContext* Context, Rewriter& R )
 : Visitor(Context, R) {}

void ReplaceIntrinsicsConsumer::HandleTranslationUnit( ASTContext& Context )
{
	Visitor.TraverseTranslationUnitDecl( Context.getTranslationUnitDecl() );
}

ReplaceIntrinsicsVisitor::ReplaceIntrinsicsVisitor( ASTContext* Context, Rewriter& R )
	: Context(Context), R(R)
{
}

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

	// TODO: replace any __readgs*/__writegs* from WDK-KM with corresponding stub 
};

bool ReplaceIntrinsicsVisitor::VisitCallExpr( CallExpr* Call )
{	
	if (const FunctionDecl* FD = Call->getDirectCallee()) {
		std::string functionName = FD->getNameAsString();
		if (IntrinsicNamesToFunctions.find( functionName ) != IntrinsicNamesToFunctions.end()) {
			std::string implementationName = IntrinsicNamesToFunctions.at(functionName);

			std::string newCall = implementationName + "(";
			newCall += Utils::DumpCallArguments( Call, Context->getPrintingPolicy() );
			newCall += ")";

			R.ReplaceText( Call->getSourceRange(), newCall );
		}
	}
	return true;
}
