#pragma once

#include "Types.h"

#include <vector>
#include <string_view>
#include <span>
#include <optional>

namespace Sig
{
	struct SigByte
	{
		Byte Value;
		bool IsWildcard;

		static SigByte Val(Byte Value) {
			return SigByte{ Value, false };
		}

		static SigByte Wildcard() {
			return SigByte{ 0, true };
		}
	};

	using Signature_t = std::span<SigByte>;

	std::vector<SigByte> FromHex(std::string_view HexString);

	std::optional<size_t> FindSignatureInBuffer(ReadonlyRegion_t Buffer, Signature_t Signature);
}

