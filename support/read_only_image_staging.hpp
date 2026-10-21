#pragma once

// Copy one complete file into RAM. The caller must then verify its identity.
#include <cstddef>
#include <cstdint>

namespace daisy_development
{
    enum class StagingResult
    {
        Staged,
        InvalidBuffer,
        NoMedia,
        OpenFailed,
        SizeMismatch,
        ReadFailed,
        ShortRead,
        Disconnected,
        CloseFailed
    };

    // Reader supplies the file operations for USB or a test. A successful copy
    // proves the length, not that the firmware is approved. Authenticate this
    // same RAM buffer before launch, and close the file on every open path.
    template <class Reader>
    StagingResult StageReadOnlyImage(Reader& reader, const char* path, std::uint8_t* buffer,
                                     std::size_t capacity, std::size_t expected_size)
    {
        if (buffer == nullptr || expected_size < 16U || expected_size > capacity)
        {
            return StagingResult::InvalidBuffer;
        }
        if (!reader.Ready())
        {
            return StagingResult::NoMedia;
        }
        if (!reader.Open(path))
        {
            return StagingResult::OpenFailed;
        }
        auto result = StagingResult::Staged;
        if (reader.Size() != expected_size)
        {
            result = StagingResult::SizeMismatch;
        }
        std::size_t offset = 0;
        while (result == StagingResult::Staged && offset < expected_size)
        {
            if (!reader.Ready())
            {
                result = StagingResult::Disconnected;
                break;
            }
            auto amount = expected_size - offset;
            if (amount > 4096U)
            {
                amount = 4096U;
            }
            std::size_t read = 0;
            if (!reader.Read(buffer + offset, amount, read))
            {
                result = StagingResult::ReadFailed;
            }
            else if (read != amount)
            {
                result = StagingResult::ShortRead;
            }
            else
            {
                offset += amount;
            }
            reader.Pump();
        }
        const bool closed = reader.Close();
        if (result == StagingResult::Staged && !closed)
        {
            result = StagingResult::CloseFailed;
        }
        if (result == StagingResult::Staged && !reader.Ready())
        {
            result = StagingResult::Disconnected;
        }
        return result;
    }
} // namespace daisy_development
