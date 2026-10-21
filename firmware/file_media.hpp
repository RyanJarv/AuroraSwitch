#pragma once
#include "media_transport.hpp"
#include "../support/boot_choice.hpp"
#include "../support/operation_timing.hpp"

namespace aurora_selector {
// Debug counters describe the last load attempt. Launch still requires
// checking the actual firmware bytes, regardless of these values.
struct LoadDiagnostic {
    std::uint32_t attempts, selection, staging_result, fatfs_result;
    std::uint32_t file_size, bytes_read, authenticated;
    daisy_development::OperationTiming timing;
};

struct FileReader {
    MediaTransport& media;
    volatile LoadDiagnostic& diagnostic;

    bool Ready() { return media.Ready(); }

    bool Open(const char* path) {
        const auto status = media.Open(path, false);
        diagnostic.fatfs_result = media.Result();
        return status == FileStatus::Ok;
    }

    std::size_t Size() {
        diagnostic.file_size = media.Size();
        return diagnostic.file_size;
    }

    bool Read(std::uint8_t* bytes, std::size_t amount, std::size_t& count) {
        const bool valid = media.Read(bytes, amount, count);
        diagnostic.fatfs_result = media.Result();
        diagnostic.bytes_read = diagnostic.bytes_read + count;
        return valid;
    }

    void Pump() { media.Pump(); }

    bool Close() {
        const bool valid = media.Close();
        if (diagnostic.fatfs_result == media.SuccessResult()) {
            diagnostic.fatfs_result = media.Result();
        }
        return valid;
    }
};

struct FileBootStore {
    MediaTransport& media;
    bool healthy = true;

    bool Healthy() const { return healthy; }

    static const char* Path(unsigned slot) {
        return slot == 0 ? "0:/aurora/boot-a.dat" : "0:/aurora/boot-b.dat";
    }

    bool Read(unsigned slot, BootRecord& record) {
        if (!media.Ready()) {
            healthy = false;
            return false;
        }
        const auto opened = media.Open(Path(slot), false);
        if (opened != FileStatus::Ok) {
            if (opened != FileStatus::Missing) {
                healthy = false;
            }
            return false;
        }
        std::size_t count = 0;
        const bool size_valid = media.Size() == record.size();
        if (size_valid &&
            (!media.Read(record.data(), record.size(), count) || count != record.size())) {
            healthy = false;
        }
        if (!media.Close() || !media.Ready()) {
            healthy = false;
        }
        return healthy && size_valid;
    }

    bool Write(unsigned slot, const BootRecord& record) {
        if (!media.Ready() || media.Open(Path(slot), true) != FileStatus::Ok) {
            return false;
        }
        std::size_t count = 0;
        const bool valid = media.Write(record.data(), record.size(), count) &&
                           count == record.size() && media.Sync();
        return media.Close() && valid && media.Ready();
    }
};
} // namespace aurora_selector
