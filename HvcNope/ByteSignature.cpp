#include "ByteSignature.h"

#include <string>

namespace Sig
{
    std::vector<SigByte> FromHex(std::string_view HexString)
    {
        std::vector<SigByte> signature;

        for (size_t i = 0; i < HexString.length(); ) {
            if (HexString[i] == '*') {
                signature.push_back(SigByte());
                ++i; // Increment by 1 for wildcard
            }
            else {
                if (i + 1 < HexString.length()) {
                    std::string_view byteString = HexString.substr(i, 2);
                    unsigned char byte = static_cast<unsigned char>(std::stoi(std::string(byteString), nullptr, 16));
                    signature.push_back(SigByte(byte));
                }
                i += 2; // Increment by 2 for a valid hex byte
            }
        }

        return signature;
    }


    std::optional<size_t> FindSignatureInBuffer(ReadonlyRegion_t Buffer, Signature_t Signature)
    {
        size_t bufferSize = Buffer.size();
        size_t sigSize = Signature.size();

        if (sigSize == 0 || bufferSize < sigSize) {
            return std::nullopt;
        }

        for (size_t i = 0; i <= bufferSize - sigSize; ++i) {
            bool match = true;

            for (size_t j = 0; j < sigSize; ++j) {
                if (!Signature[j].IsWildcard && Buffer[i + j] != Signature[j].Value) {
                    match = false;
                    break;
                }
            }

            if (match) {
                return i;
            }
        }

        return std::nullopt; // Signature not found
    }
}


