#include "pch.h"
#include "ReplaceIntrinisicsConsumer.h"
#include "Utils.h"
#include "ToolConfiguration.h"

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
			std::string kernelGsWrapper = GsIntrinsicNamesToFunctions.at( calleeName );
			
			SourceManager& SM = Context->getSourceManager();

			outs() << "Callee name: " << calleeName << "\n";
			Call->getSourceRange().dump( SM );

			// check if CallExpr is inside known GS source range
			for ( SourceRange gsRange : KernelGsSourceRanges )
			{
				outs() << "GS known range:\n";
				gsRange.dump(SM);

				if (gsRange.fullyContains( Call->getSourceRange() ) ) {
					outs() << "Returning range\n";
					return kernelGsWrapper;
				}
			}

			// if CallExpr is in kernel source, assume we're dealing with kernel GS
			std::string fileName = SM.getFilename( Call->getBeginLoc() ).str();

			if (ToolConfiguration::Instance().IsKernelSource( fileName )) {
				return kernelGsWrapper;
			}
		}
	}
	return std::nullopt;
}

bool ReplaceIntrinsicsVisitor::VisitCallExpr( CallExpr* Call )
{	
	VISITOR_SKIP_NON_MAIN_FILES( Call, Context );

	if (const FunctionDecl* FD = Call->getDirectCallee()) {
		auto wrapperName = GetIntrinsicWrapperNameIfNecessary( Call );
		if (wrapperName) {
			// replace call
			std::string newCall = wrapperName.value() + "(";
			newCall += Utils::Clang::DumpCallArguments( Call, Context->getPrintingPolicy() );
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
	VISITOR_SKIP_NON_MAIN_FILES( FD, Context );

	SourceManager& SM = Context->getSourceManager();
	if (!SM.isInMainFile( FD->getBeginLoc() )) return true;

	if (Utils::Clang::HasAnnotateAttrWithName( FD, "kernel_gs" )) {

		KernelGsSourceRanges.push_back( FD->getSourceRange() );
	}
	else {
		auto currentFileName = SM.getFilename( FD->getBeginLoc() );
		if (currentFileName.find( "main.cpp" ) != StringRef::npos) {
		}
	}

	return true;
}
