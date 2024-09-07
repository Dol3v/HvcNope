#include "ByteSignature.h"

#include <sstream>
#include <string>
#include "Log.h"

namespace Sig
{
    std::vector<SigByte> FromHex(std::string_view HexString)
    {
        std::vector<SigByte> signature;

        for (size_t i = 0; i < HexString.length(); ) 
        {
            if (isspace( HexString[i] )) {
                ++i;
            } else if (HexString[i] == '*') {
                signature.push_back(SigByte::Wildcard());
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

    std::vector<SigByte> FromBytes(std::span<const Byte> Bytes)
    {
        std::vector<SigByte> result;

        for (size_t i = 0; i < Bytes.size(); i++)
        {
            result.push_back(SigByte(Bytes[i]));
        }
        return result;
    }

    Signature_t FromVector( std::vector<Sig::SigByte>& Vec )
    {
        return Signature_t(Vec.data(), Vec.size());
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

    std::string HexDump(Signature_t Signature)
    {
        std::stringstream ss;

        for (auto sigbyte : Signature)
        {
            if (sigbyte.IsWildcard) {
                ss << "*";
            }
            else {
                ss << std::hex << sigbyte.Value;
            }

            ss << " ";
        }

        return ss.str();
    }
}


