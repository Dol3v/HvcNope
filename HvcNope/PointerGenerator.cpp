#include "PointerGenerator.h"

PointerGenerator::PointerGenerator()
{

}

kAddress PointerGenerator::GenerateFunctionPointer(void* UsermodeFunction)
{
	return kAddress();
}

PointerGenerator::~PointerGenerator()
{
}

std::optional<kAddress> PointerGenerator::RemapLongJmpGadget()
{
	return std::optional<kAddress>();
}

bool PointerGenerator::InsertFunctionPointer(kAddress MappedGadget, const UsermodeFunction& Function)
{
	return false;
}

bool PointerGenerator::CraftFunctionPointerStack(kAddress JmpBuf)
{
	return false;
}

bool PointerGenerator::LocateLongjmpGadgetAndBuf()
{
	return false;
}
