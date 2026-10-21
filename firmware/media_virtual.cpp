// Synthetic low-level transport for bounded linked execution, never real USB.
#include "media_transport.hpp"
#include "images.hpp"
#include "../support/backed_image_reader.hpp"
#include <cstring>

extern "C" {
volatile daisy_development::BackedImageDescriptor selector_virtual_media{};
struct VirtualBootStore
{
    std::uint32_t version, present, writes, fail_writes;
    std::uint8_t records[2][80];
};
volatile VirtualBootStore selector_virtual_boot_store{1, 0, 0, 0, {}};
}

namespace
{
    daisy_development::BackedImageReader reader(selector_virtual_media,
        reinterpret_cast<const std::uint8_t*>(SELECTOR_VIRTUAL_FILE_BASE), aurora_selector::StagingCapacity());
    int record_slot = -1;
    bool writing = false;
}

namespace aurora_selector
{
    daisy_development::MediaInitialization MediaTransport::Initialize()
    { return daisy_development::MediaInitialization::Ready; }
    bool MediaTransport::Ready() { return reader.Ready(); }
    void MediaTransport::Pump() { reader.Pump(); }
    FileStatus MediaTransport::Open(const char* path, bool write)
    {
        record_slot = std::strcmp(path, "0:/aurora/boot-a.dat") == 0 ? 0
            : std::strcmp(path, "0:/aurora/boot-b.dat") == 0 ? 1 : -1;
        writing = write;
        if(record_slot < 0)
            return !write && reader.Open(path) ? FileStatus::Ok : FileStatus::Error;
        if(write) selector_virtual_boot_store.writes = selector_virtual_boot_store.writes + 1U;
        if(selector_virtual_boot_store.version != 1 || (write && selector_virtual_boot_store.fail_writes))
            return FileStatus::Error;
        return write || (selector_virtual_boot_store.present & (1U << record_slot))
            ? FileStatus::Ok : FileStatus::Missing;
    }
    std::size_t MediaTransport::Size() { return record_slot < 0 ? reader.Size() : 80; }
    bool MediaTransport::Read(std::uint8_t* bytes, std::size_t amount, std::size_t& count)
    {
        if(record_slot < 0) return reader.Read(bytes, amount, count);
        count = 0;
        if(writing || amount != 80 || !Ready()) return false;
        for(unsigned i = 0; i < 80; ++i) bytes[i] = selector_virtual_boot_store.records[record_slot][i];
        count = amount;
        return true;
    }
    bool MediaTransport::Write(const std::uint8_t* bytes, std::size_t amount, std::size_t& count)
    {
        count = 0;
        if(record_slot < 0 || !writing || amount != 80 || !Ready()) return false;
        for(unsigned i = 0; i < 80; ++i) selector_virtual_boot_store.records[record_slot][i] = bytes[i];
        selector_virtual_boot_store.present = selector_virtual_boot_store.present | (1U << record_slot);
        count = amount;
        return true;
    }
    bool MediaTransport::Sync() { return Ready(); }
    bool MediaTransport::Close() { return record_slot < 0 ? reader.Close() : Ready(); }
    // No FatFs result is claimed by a synthetic transport.
    std::uint32_t MediaTransport::Result() { return SELECTOR_DIAGNOSTIC_UNAVAILABLE; }
    std::uint32_t MediaTransport::SuccessResult() { return SELECTOR_DIAGNOSTIC_UNAVAILABLE; }
    bool MediaTransport::Unmount() { return Ready(); }
    void MediaTransport::Stop() { selector_virtual_media.ready = 0; }
}
