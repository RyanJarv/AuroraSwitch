// Synthetic transport controls. Executes the exact helper used by firmware.
#include "../support/read_only_image_staging.hpp"
#include <array>
#include <cassert>
#include <cstring>

namespace
{
    constexpr std::size_t Size = 8193;
    // Inject transport failures and count cleanup calls without a filesystem.
    struct Reader
    {
        bool ready = true, open_ok = true, close_ok = true, read_ok = true;
        bool short_read = false, disconnect_on_pump = false, disconnect_on_close = false;
        std::size_t size = ::Size, opens = 0, closes = 0, reads = 0, pumps = 0;
        std::size_t offset = 0;
        bool Ready() { return ready; }
        bool Open(const char*) { ++opens; return open_ok; }
        std::size_t Size() { return size; }
        bool Read(std::uint8_t* dst, std::size_t amount, std::size_t& read)
        {
            ++reads;
            assert(amount <= 4096 && offset + amount <= size);
            read = short_read ? amount - 1 : amount;
            if(read_ok)
                std::memset(dst, static_cast<int>(reads), read);
            offset += read;
            return read_ok;
        }
        void Pump() { ++pumps; if(disconnect_on_pump) ready = false; }
        bool Close() { ++closes; if(disconnect_on_close) ready = false; return close_ok; }
    };
}

int main()
{
    using daisy_development::StagingResult;
    std::array<std::uint8_t, Size + 2> buffer{};
    buffer.front() = 0xaa;
    buffer.back() = 0xbb;
    const auto run = [&](Reader& r, std::size_t capacity = Size) {
        return daisy_development::StageReadOnlyImage(
            r, "synthetic", buffer.data() + 1, capacity, Size);
    };
    Reader good;
    assert(run(good) == StagingResult::Staged);
    assert(good.reads == 3 && good.pumps == 3 && good.opens == 1 && good.closes == 1);
    assert(buffer[1] == 1 && buffer[4097] == 2 && buffer[8193] == 3);
    assert(buffer.front() == 0xaa && buffer.back() == 0xbb);
    Reader small;
    assert(run(small, Size - 1) == StagingResult::InvalidBuffer);
    assert(small.opens == 0);
    Reader absent;
    absent.ready = false;
    assert(run(absent) == StagingResult::NoMedia && absent.opens == 0);
    Reader bad_open;
    bad_open.open_ok = false;
    assert(run(bad_open) == StagingResult::OpenFailed && bad_open.closes == 0);
    for(const auto size : {Size - 1, Size + 1})
    {
        Reader mismatch;
        mismatch.size = size;
        assert(run(mismatch) == StagingResult::SizeMismatch);
        assert(mismatch.reads == 0 && mismatch.closes == 1);
    }
    Reader read_error;
    read_error.read_ok = false;
    assert(run(read_error) == StagingResult::ReadFailed && read_error.closes == 1);
    Reader truncated;
    truncated.short_read = true;
    assert(run(truncated) == StagingResult::ShortRead && truncated.closes == 1);
    Reader disconnected;
    disconnected.disconnect_on_pump = true;
    assert(run(disconnected) == StagingResult::Disconnected);
    assert(disconnected.reads == 1 && disconnected.closes == 1);
    Reader close_error;
    close_error.close_ok = false;
    assert(run(close_error) == StagingResult::CloseFailed && close_error.closes == 1);
    Reader late_disconnect;
    late_disconnect.disconnect_on_close = true;
    assert(run(late_disconnect) == StagingResult::Disconnected);
    Reader null_buffer;
    assert(daisy_development::StageReadOnlyImage(
        null_buffer, "synthetic", nullptr, Size, Size) == StagingResult::InvalidBuffer);
    assert(null_buffer.opens == 0);
    assert(buffer.front() == 0xaa && buffer.back() == 0xbb);
}
