#pragma once
#include <cstddef>
#include <cstdint>
#include "../support/media_initialization.hpp"

namespace aurora_selector
{
    enum class FileStatus { Ok, Missing, Error };

    // One open file, backed by the existing DMA-safe FatFs objects on hardware.
    // The build links one implementation; selection and file policy are shared.
    struct MediaTransport
    {
        daisy_development::MediaInitialization Initialize();
        bool Ready();
        void Pump();
        FileStatus Open(const char* path, bool write);
        std::size_t Size();
        bool Read(std::uint8_t* bytes, std::size_t amount, std::size_t& count);
        bool Write(const std::uint8_t* bytes, std::size_t amount, std::size_t& count);
        bool Sync();
        bool Close();
        std::uint32_t Result();
        std::uint32_t SuccessResult();
        bool Unmount();
        void Stop();
    };
}
