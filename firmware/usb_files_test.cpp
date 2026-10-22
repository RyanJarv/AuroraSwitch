#include "usb_files.hpp"
#include <cassert>
#include <map>
#include <string>
#include <vector>

std::map<std::string, std::vector<std::uint8_t>> files;
bool connected = true, fail_write = false, fail_sync = false, fail_close = false;
bool short_read = false, short_write = false, corrupt_readback = false;
unsigned writes = 0, creates = 0, reads = 0;
FRESULT f_stat(const char* path, FILINFO* info) {
    if (!connected)
        return FR_DISK_ERR;
    if (!files.count(path))
        return FR_NO_FILE;
    info->fsize = files[path].size();
    info->fattrib = 0;
    return FR_OK;
}
FRESULT f_open(FIL* file, const char* path, unsigned mode) {
    if (!connected)
        return FR_DISK_ERR;
    if (mode & FA_CREATE_ALWAYS) {
        files[path].clear();
        ++creates;
    }
    if (!files.count(path))
        return FR_NO_FILE;
    *file = {path, 0, files[path].size()};
    return FR_OK;
}
FRESULT f_read(FIL* file, void* output, UINT count, UINT* read) {
    ++reads;
    if (!connected)
        return FR_DISK_ERR;
    auto& data = files[file->path];
    *read = static_cast<UINT>(std::min<std::size_t>(count, data.size() - file->position));
    if (short_read && *read)
        --*read;
    std::memcpy(output, data.data() + file->position, *read);
    file->position += *read;
    return FR_OK;
}
FRESULT f_write(FIL* file, const void* input, UINT count, UINT* written) {
    ++writes;
    if (!connected || fail_write)
        return FR_DISK_ERR;
    *written = count - (short_write ? 1 : 0);
    auto* bytes = static_cast<const std::uint8_t*>(input);
    files[file->path].assign(bytes, bytes + *written);
    if (corrupt_readback)
        files[file->path].back() ^= 1;
    return FR_OK;
}
FRESULT f_sync(FIL*) {
    return fail_sync ? FR_DISK_ERR : FR_OK;
}
FRESULT f_close(FIL*) {
    return fail_close ? FR_DISK_ERR : FR_OK;
}
int main() {
    using namespace aurora_selector;
    assert(ReadChoice() == -1);
    assert(!WriteChoice(ImageCount) && writes == 0);
    for (unsigned index = 0; index < ImageCount; ++index) {
        const auto record = EncodeChoice(index);
        assert(DecodeChoice(record) == static_cast<int>(index));
        for (unsigned byte = 0; byte < record.size(); ++byte) {
            auto torn = record;
            torn[byte] ^= 1;
            assert(DecodeChoice(torn) == -1);
        }
        assert(WriteChoice(index));
        const auto previous = writes;
        assert(ReadChoice() == static_cast<int>(index));
        for (unsigned repeat = 0; repeat < 100; ++repeat)
            assert(WriteChoice(index));
        assert(writes == previous);
    }
    assert(writes == ImageCount && creates == ImageCount);
    // Every save error is reported; the controller owns any explicit retry.
    for (auto* failure : {&fail_write, &short_write, &fail_sync, &fail_close, &corrupt_readback}) {
        files.erase(ChoicePath);
        *failure = true;
        assert(!WriteChoice(0));
        *failure = false;
    }
    for (unsigned size : {0U, 4U, 75U, 77U, 1000U}) {
        files[ChoicePath].assign(size, 0);
        assert(ReadChoice() == -1);
    }
    // Correct CRC with an unsupported hash still cannot select anything.
    auto unknown = EncodeChoice(0);
    std::memset(unknown.data() + 8, 'a', 64);
    const auto crc = ChoiceCrc(unknown.data(), 72);
    for (unsigned byte = 0; byte < 4; ++byte)
        unknown[72 + byte] = crc >> (8 * byte);
    assert(DecodeChoice(unknown) == -1);
    char path[128];
    FirmwarePath(path, sizeof(path), 0);
    files[path].assign(Images[0].size, 0x5a);
    std::array<bool, ImageCount> present{};
    DiscoverFiles(present);
    assert(present[0]);
    std::vector<std::uint8_t> staged(StagingCapacity(), 0);
    assert(ReadOriginal(0, staged.data(), staged.size()));
    assert(staged[0] == 0x5a && staged[Images[0].size - 1] == 0x5a);
    assert(!ReadOriginal(0, staged.data(), Images[0].size - 1));
    assert(!ReadOriginal(ImageCount, staged.data(), staged.size()));
    short_read = true;
    assert(!ReadOriginal(0, staged.data(), staged.size()));
    short_read = false;
    files[path].pop_back();
    assert(!ReadOriginal(0, staged.data(), staged.size()));
    files[path].resize(Images[0].size + 1);
    assert(!ReadOriginal(0, staged.data(), staged.size()));
    connected = false;
    assert(!ReadOriginal(0, staged.data(), staged.size()));
    assert(!WriteChoice(0) && ReadChoice() == -1);
    connected = true;
    files[path].resize(Images[0].size);
    assert(ReadOriginal(0, staged.data(), staged.size()));
}
