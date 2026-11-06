#include "usb_files.hpp"
#include <cassert>
#include <map>
#include <string>
#include <vector>

std::map<std::string, std::vector<std::uint8_t>> files;
bool connected = true, fail_write = false, fail_sync = false, fail_close = false;
bool short_read = false, short_write = false, corrupt_readback = false;
unsigned writes = 0, creates = 0, reads = 0;
bool fail_directory = false, fail_rename = false;
unsigned renames = 0, directory_closes = 0;
std::map<std::string, std::string> aliases;
std::map<std::string, unsigned> attributes;
FRESULT f_opendir(DIR* directory, const char*) {
    directory->position = 0;
    return connected && !fail_directory ? FR_OK : FR_DISK_ERR;
}
FRESULT f_readdir(DIR* directory, FILINFO* info) {
    if (!connected || fail_directory)
        return FR_DISK_ERR;
    std::vector<std::string> root;
    for (const auto& entry : files)
        if (entry.first.find('/', 3) == std::string::npos)
            root.push_back(entry.first);
    *info = {};
    if (directory->position < root.size()) {
        const auto path = root[directory->position++];
        info->fattrib = attributes[path];
        std::snprintf(info->fname, sizeof(info->fname), "%s", path.c_str() + 3);
        std::snprintf(info->altname, sizeof(info->altname), "%s", aliases[path].c_str());
    }
    return FR_OK;
}
FRESULT f_closedir(DIR*) {
    ++directory_closes;
    return connected ? FR_OK : FR_DISK_ERR;
}
FRESULT f_rename(const char* source, const char* target) {
    ++renames;
    if (!connected || fail_rename)
        return FR_DISK_ERR;
    if (!files.count(source))
        return FR_NO_FILE;
    if (files.count(target))
        return FR_EXIST;
    files[target] = files[source];
    attributes[target] = attributes[source];
    files.erase(source);
    attributes.erase(source);
    return FR_OK;
}
FRESULT f_stat(const char* path, FILINFO* info) {
    if (!connected)
        return FR_DISK_ERR;
    if (!files.count(path))
        return FR_NO_FILE;
    info->fsize = files[path].size();
    info->fattrib = attributes[path];
    return FR_OK;
}
FRESULT f_open(FIL* file, const char* path, unsigned mode) {
    if (!connected)
        return FR_DISK_ERR;
    if ((mode & FA_CREATE_NEW) && files.count(path))
        return FR_EXIST;
    if (mode & (FA_CREATE_ALWAYS | FA_CREATE_NEW)) {
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
    auto& data = files[file->path];
    data.resize(file->position + *written);
    std::copy(bytes, bytes + *written, data.begin() + file->position);
    file->position += *written;
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
    unsigned ram_images = 0;
    ChoiceRecord record{};
    for (unsigned index = 0; index < ImageCount; ++index) {
        const auto size = EncodeChoice(index, record);
        if (Images[index].launch != LaunchMethod::Ram) {
            assert(!WriteChoice(index));
            assert(size == 0 && DecodeChoice(record, size) == -1);
            // A manually edited filename still cannot select an installation.
            const auto length = std::strlen(Images[index].filename);
            std::memcpy(record.data(), Images[index].filename, length);
            assert(DecodeChoice(record, length) == -1);
            continue;
        }
        ++ram_images;
        assert(DecodeChoice(record, size) == static_cast<int>(index));
        assert(WriteChoice(index));
        const auto previous = writes;
        assert(ReadChoice() == static_cast<int>(index));
        // Accept ordinary editor line endings without normalizing/re-writing.
        const auto* name = Images[index].filename;
        const std::string filename(name);
        for (const auto* ending : {"", "\n", "\r\n"}) {
            const auto text = filename + ending;
            files[ChoicePath].assign(text.begin(), text.end());
            assert(ReadChoice() == static_cast<int>(index));
            for (unsigned repeat = 0; repeat < 100; ++repeat)
                assert(WriteChoice(index));
            assert(writes == previous);
        }
    }
    assert(writes == ram_images && creates == ram_images);
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
    for (const auto* text : {"unknown.bin\n", "../Aurora_v1_4_4.bin", "Aurora_v1_4_4.bin\nextra",
                             "Aurora_v1_4_4.bin ", "Aurora_v1_4_4.bi"}) {
        const std::string invalid(text);
        files[ChoicePath].assign(invalid.begin(), invalid.end());
        assert(ReadChoice() == -1);
    }
    assert(DecodeChoice(record, record.size() + 1) == -1);
    char path[128];
    FirmwarePath(path, sizeof(path), 0);
    files[path].assign(Images[0].size, 0x5a);
    std::array<bool, ImageCount> present{};
    DiscoverFiles(present);
    assert(present[0]);
    std::vector<std::uint8_t> staged(StagingCapacity(), 0);
    assert(ReadImageFile(path, 0, staged.data(), staged.size()));
    assert(staged[0] == 0x5a && staged[Images[0].size - 1] == 0x5a);
    assert(!ReadImageFile(path, 0, staged.data(), Images[0].size - 1));
    assert(!ReadImageFile(path, ImageCount, staged.data(), staged.size()));
    short_read = true;
    assert(!ReadImageFile(path, 0, staged.data(), staged.size()));
    short_read = false;
    files[path].pop_back();
    assert(!ReadImageFile(path, 0, staged.data(), staged.size()));
    files[path].resize(Images[0].size + 1);
    assert(!ReadImageFile(path, 0, staged.data(), staged.size()));
    connected = false;
    assert(!ReadImageFile(path, 0, staged.data(), staged.size()));
    assert(!WriteChoice(0) && ReadChoice() == -1);
    connected = true;
    files[path].resize(Images[0].size);
    assert(ReadImageFile(path, 0, staged.data(), staged.size()));
    // Root conflicts include substring matches and short-name aliases. Never
    // overwrite an existing file, even one with the expected install name.
    files.clear();
    assert(CheckInstallRoot());
    for (const auto* name :
         {"AuroraSwitch.bin", "OTHER.BIN", "other.BiN.disabled", "DIRTVERB.BIN"}) {
        const std::string path = std::string("0:/") + name;
        files[path] = {1, 2, 3};
        assert(!CheckInstallRoot());
        files.erase(path);
    }
    files["0:/harmless.img"] = {1};
    assert(CheckInstallRoot());
    aliases["0:/harmless.img"] = "HIDDEN.BIN";
    assert(!CheckInstallRoot());
    aliases.clear();
    fail_directory = true;
    assert(!CheckInstallRoot());
    fail_directory = false;
    const unsigned dirt = ImageCount - 1;
    FirmwarePath(path, sizeof(path), dirt);
    files[path].assign(Images[dirt].size, 0x3a);
    const auto original = files[path];
    attributes[path] = AM_HID;
    assert(!CheckInstallFiles(dirt) && renames == 0 && files[path] == original);
    attributes[path] = 0;
    assert(CheckInstallFiles(dirt));
    std::vector<std::uint8_t> selector(10003);
    for (std::size_t i = 0; i < selector.size(); ++i)
        selector[i] = i % 251;
    auto preflight = [&] {
        return CheckReturnFiles(selector.data(), selector.size(), staged.data(), staged.size());
    };
    auto prepare = [&] {
        return PrepareReturnFiles(selector.data(), selector.size(), staged.data(), staged.size());
    };
    files[SelectorPath] = selector;
    assert(CheckInstallFiles(dirt) && preflight() && renames == 0);
    files[SelectorPath].back() ^= 1;
    assert(!preflight() && renames == 0);
    files[SelectorPath] = selector;
    files[SelectorBackupPath] = {1};
    assert(!preflight()); // Never overwrite an existing backup.
    files.erase(SelectorBackupPath);
    files[ReturnPath] = {1};
    assert(!preflight()); // Nor an unrelated file with the instructions' name.
    files.erase(ReturnPath);
    fail_rename = true;
    assert(!prepare() && files[SelectorPath] == selector && !files.count(InstallPath));
    fail_rename = false;
    assert(prepare());
    assert(!files.count(SelectorPath) && files[SelectorBackupPath] == selector);
    const std::vector<std::uint8_t> instructions(ReturnBytes(),
                                                 ReturnBytes() + sizeof(ReturnInstructions) - 1);
    assert(files[ReturnPath] == instructions);
    assert(files[path] == original && !files.count(InstallPath));
    attributes[SelectorBackupPath] = AM_HID;
    assert(!preflight()); // A hidden backup would stay hidden when re-enabled.
    attributes[SelectorBackupPath] = 0;
    const auto saved_writes = writes;
    assert(prepare() && writes == saved_writes); // Reuse exact backup/text without rewriting.
    assert(ReadImageFile(path, dirt, staged.data(), staged.size()));
    assert(
        WriteNewFile(InstallPath, staged.data(), Images[dirt].size, staged.data(), staged.size()));
    assert(files[path] == original && files[InstallPath] == original);
    assert(CheckInstallRoot(InstallRoot::DirtOnly) && !CheckInstallRoot());
    assert(ReadImageFile(InstallPath, dirt, staged.data(), staged.size()));
    assert(
        !WriteNewFile(InstallPath, selector.data(), selector.size(), staged.data(), staged.size()));
    assert(files[InstallPath] == original);
    assert(files["0:/harmless.img"] == std::vector<std::uint8_t>{1});
    connected = false;
    assert(!CheckInstallRoot(InstallRoot::DirtOnly));
    connected = true;
    attributes[InstallPath] = AM_HID;
    assert(!CheckInstallRoot(InstallRoot::DirtOnly));
    files.erase(InstallPath);
    attributes.erase(InstallPath);
    // Removed installer: reconstruct the same complete backup from installed bytes.
    files.erase(SelectorBackupPath);
    files.erase(ReturnPath);
    assert(prepare() && files[SelectorBackupPath] == selector && files[ReturnPath] == instructions);
    for (auto* failure : {&fail_write, &short_write, &fail_sync, &fail_close, &corrupt_readback}) {
        files.erase(SelectorBackupPath);
        *failure = true;
        assert(!prepare());
        *failure = false;
    }
}
