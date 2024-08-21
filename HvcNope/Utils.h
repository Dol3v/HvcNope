#pragma once

#include "Types.h"

constexpr bool cStopLoop = false;
constexpr bool cContinueLoop = true;

/*
	Calls a consumer for every entry of a _LIST_ENTRY structure.

	The consumer receives the address of the LIST_ENTRY itself, for a version
	that receives the address of the structure start (which may be different) see
	ForEveryListElement.

	Note that if the consumer returns false, the loop will stop.
*/
template <typename ConsumerType>
void ForEveryListEntry(_In_ kAddress ListHead, _In_ ConsumerType Consumer)
{
	auto flink = g_Rw->ReadQword(ListHead);
	while (flink != ListHead) 
	{
		if (Consumer(flink) == cStopLoop) {
			// the consumer wants to stop the loop
			return;
		}
		flink = g_Rw->ReadQword(ListHead); // flink->Flink
	}
}

template <typename ConsumerType>
void ForEveryListElement(_In_ kAddress ListHead, _In_ Qword EntryOffset, _In_ ConsumerType Consumer)
{
	auto flink = g_Rw->ReadQword(ListHead);
	while (flink != ListHead)
	{
		if (Consumer(flink - EntryOffset) == cStopLoop) {
			return;
		}
		flink = g_Rw->ReadQword(ListHead); // flink->Flink
	}
}
