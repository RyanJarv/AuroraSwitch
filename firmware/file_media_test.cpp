// Execute the production USB adapter with scripted SDK/FatFs calls.
#include "file_media.hpp"
#include "../support/read_only_image_staging.hpp"
#include "aurora.h"
#include "../tests/fakes/file_media_before_refactor.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <map>
#include <string>
#include <vector>
using namespace aurora_selector;
namespace
{
    struct Io
    {
        std::map<std::string, std::vector<std::uint8_t>> files;
        std::vector<std::string> calls;
        std::map<std::string, unsigned> counts;
        std::string fail, drop, path;
        unsigned fail_at = 1, drop_at = 1;
        bool ready = true, opened = false, short_read = false, short_write = false;
        BYTE flags = 0;
        std::size_t offset = 0;
        bool Call(const char* op)
        {
            calls.emplace_back(op);
            const auto n = ++counts[op];
            if(drop == op && n == drop_at) ready = false;
            return fail != op || n != fail_at;
        }
    } io;
    const char* A = "0:/aurora/boot-a.dat";
    const char* B = "0:/aurora/boot-b.dat";
    MediaTransport media;
    void Reset()
    {
        io = {};
        before_refactor::load_diagnostic.fatfs_result = 0;
        before_refactor::load_diagnostic.bytes_read = 0;
        before_refactor::load_diagnostic.file_size = 0;
    }
}
namespace aurora { daisy::USBHostHandle usb; }
daisy::USBHostHandle::Result daisy::USBHostHandle::Init(Config)
{ return io.Call("usb-init") ? Result::OK : Result::ERR; }
bool daisy::USBHostHandle::GetReady() { return io.ready; }
void daisy::USBHostHandle::Process() { io.Call("pump"); }
void daisy::USBHostHandle::Deinit() { io.Call("stop"); io.ready = false; }
daisy::FatFSInterface::Result daisy::FatFSInterface::Init(Config config)
{
    assert(config.media == Config::MEDIA_USB);
    assert(reinterpret_cast<std::uintptr_t>(this) % 32 == 0);
    return io.Call("fs-init") ? Result::OK : Result::ERR;
}
FRESULT f_mount(FATFS* fs, const char* path, BYTE lazy)
{
    assert(std::strcmp(path, "0:") == 0 && lazy == 0);
    return io.Call(fs ? "mount" : "unmount") ? FR_OK : FR_DISK_ERR;
}
FRESULT f_open(FIL* file, const char* path, BYTE flags)
{
    assert(reinterpret_cast<std::uintptr_t>(file) % 32 == 0);
    assert(!io.opened);
    if(!io.Call("open")) return FR_DISK_ERR;
    io.path = path; io.offset = 0; io.flags = flags;
    if(flags == (FA_WRITE | FA_CREATE_ALWAYS)) io.files[path].clear();
    else { assert(flags == FA_READ); if(!io.files.count(path)) return FR_NO_FILE; }
    io.opened = true;
    file->size = io.files[path].size();
    return FR_OK;
}
FRESULT f_read(FIL*, void* bytes, UINT amount, UINT* count)
{
    assert(io.opened && io.flags == FA_READ);
    *count = 0;
    if(!io.Call("read")) return FR_DISK_ERR;
    const auto& data = io.files.at(io.path);
    *count = std::min<std::size_t>(amount, data.size() - io.offset);
    if(io.short_read && *count) --*count;
    std::memcpy(bytes, data.data() + io.offset, *count);
    io.offset += *count;
    return FR_OK;
}
FRESULT f_write(FIL*, const void* bytes, UINT amount, UINT* count)
{
    assert(io.opened && io.flags == (FA_WRITE | FA_CREATE_ALWAYS));
    *count = 0;
    if(!io.Call("write")) return FR_DISK_ERR;
    *count = io.short_write ? amount - 1 : amount;
    const auto* data = static_cast<const std::uint8_t*>(bytes);
    io.files[io.path].insert(io.files[io.path].end(), data, data + *count);
    return FR_OK;
}
FRESULT f_sync(FIL*)
{ assert(io.opened); return io.Call("sync") ? FR_OK : FR_DISK_ERR; }
FRESULT f_close(FIL*)
{ assert(io.opened); io.opened = false; return io.Call("close") ? FR_OK : FR_DISK_ERR; }

namespace
{
    struct Observation
    {
        bool success, healthy;
        BootRecord record;
        std::map<std::string, std::vector<std::uint8_t>> files;
        std::vector<std::string> calls;
        bool operator==(const Observation& other) const
        { return success == other.success && healthy == other.healthy && record == other.record
            && files == other.files && calls == other.calls; }
    };
    template<class Store> Observation RecordOperation(Store& store, bool write)
    {
        BootRecord record{};
        const bool result = write ? store.Write(0, record) : store.Read(0, record);
        return {result, store.Healthy(), record, io.files, io.calls};
    }
    void CompareRecordPolicies()
    {
        for(bool write : {false, true})
            for(const auto* failure : {"", "open", "read", "write", "sync", "close"})
                for(const auto* drop : {"", "read", "write", "sync", "close"})
                    for(unsigned shape = 0; shape < 5; ++shape)
                    {
                        Reset();
                        if(shape != 0) io.files[A] = std::vector<std::uint8_t>(shape == 1 ? 79 : 80, 0x35);
                        io.short_read = shape == 3; io.short_write = shape == 4;
                        io.fail = failure; io.drop = drop;
                        const auto initial = io;
                        before_refactor::BootStore old;
                        const auto expected = RecordOperation(old, write);
                        io = initial;
                        FileBootStore current{media};
                        assert(RecordOperation(current, write) == expected);
                        assert(!io.opened);
                    }
    }
    void CompareImagePolicies()
    {
        using daisy_development::StageReadOnlyImage;
        for(const auto* failure : {"", "open", "read", "close"})
            for(const auto* drop : {"", "open", "read", "pump", "close"})
                for(unsigned occurrence : {1U, 2U, 3U})
                    for(bool short_read : {false, true})
                    {
                        Reset();
                        io.files["image"] = std::vector<std::uint8_t>(8193, 0x5a);
                        io.fail = failure; io.drop = drop;
                        io.fail_at = io.drop_at = occurrence; io.short_read = short_read;
                        const auto initial = io;
                        std::vector<std::uint8_t> expected(8193, 0), actual(8193, 0);
                        before_refactor::UsbFileReader old;
                        const auto old_result = StageReadOnlyImage(old, "image", expected.data(), expected.size(), expected.size());
                        const auto old_calls = io.calls;
                        const auto old_error = before_refactor::load_diagnostic.fatfs_result;
                        const auto old_count = before_refactor::load_diagnostic.bytes_read;
                        io = initial;
                        volatile LoadDiagnostic diagnostic{};
                        FileReader current{media, diagnostic};
                        assert(StageReadOnlyImage(current, "image", actual.data(), actual.size(), actual.size()) == old_result);
                        assert(io.calls == old_calls && actual == expected && !io.opened);
                        assert(diagnostic.fatfs_result == old_error && diagnostic.bytes_read == old_count);
                    }
    }
}
int main()
{
    using daisy_development::MediaInitialization;
    for(const auto* failure : {"", "usb-init", "fs-init", "mount"})
    {
        Reset(); io.fail = failure;
        const auto result = media.Initialize();
        assert(result == (io.fail.empty() ? MediaInitialization::Ready
            : io.fail == "usb-init" ? MediaInitialization::UsbFailed
            : io.fail == "fs-init" ? MediaInitialization::FilesystemFailed : MediaInitialization::MountFailed));
        assert(io.calls.size() == (io.fail == "usb-init" ? 1U : io.fail == "fs-init" ? 2U : 3U));
    }
    CompareRecordPolicies();
    CompareImagePolicies();
    // Persistence uses the real file open/write/sync/close/readback route.
    Reset();
    FileBootStore store{media};
    assert(RememberBootChoice(store, 0));
    assert((io.calls == std::vector<std::string>{"open", "open", "open", "write", "sync", "close", "open", "read", "close"}));
    assert(io.files.count(A) && !io.files.count(B));
    for(unsigned i = 0; i < 100; ++i) assert(RememberBootChoice(store, 0));
    assert(io.counts["write"] == 1);
    assert(RememberBootChoice(store, 1));
    assert(io.files.count(B) && io.counts["write"] == 2);
    // A torn replacement cannot destroy the other slot's last valid choice.
    io.short_write = true;
    assert(!RememberBootChoice(store, 2));
    assert(ReadBootChoice(store).index == 1 && !io.opened);
    // I/O errors prevent both adoption and a subsequent attempted overwrite.
    for(const auto* failure : {"open", "read", "close"})
    {
        io.fail = failure; io.counts.clear();
        FileBootStore failed{media};
        assert(ReadBootChoice(failed).index == -1);
        assert(!RememberBootChoice(failed, 2) && io.counts["write"] == 0);
    }
    Reset(); io.ready = false;
    FileBootStore absent{media}; BootRecord record{};
    assert(!absent.Read(0, record) && !absent.Healthy() && io.calls.empty());
    assert(!absent.Write(0, record) && io.calls.empty());
    Reset(); io.fail = "unmount";
    assert(!media.Unmount() && io.calls == std::vector<std::string>{"unmount"});
    Reset(); assert(media.Unmount()); media.Stop();
    assert((io.calls == std::vector<std::string>{"unmount", "stop"}) && !media.Ready());
}
