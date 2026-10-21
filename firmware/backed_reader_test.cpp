// Synthetic transport controls; no USB/FatFs claim.
#include "../support/backed_image_reader.hpp"
#include "../support/read_only_image_staging.hpp"
#include <array>
#include <cassert>
#include <cstring>

int main()
{
    using namespace daisy_development;
    std::array<std::uint8_t, 8193> source{}, destination{};
    for(std::size_t i = 0; i < source.size(); ++i)
        source[i] = static_cast<std::uint8_t>(i);
    BackedImageDescriptor d{};
    auto reset = [&] {
        d = {};
        d.version = d.ready = d.generation = 1;
        d.size = source.size();
        std::strcpy(d.path, "image.bin");
        destination.fill(0);
    };
    auto stage = [&] {
        BackedImageReader reader(d, source.data(), source.size());
        return StageReadOnlyImage(reader, "image.bin", destination.data(),
                                 destination.size(), source.size());
    };
    reset();
    assert(stage() == StagingResult::Staged && source == destination);
    reset(); d.ready = 0; assert(stage() == StagingResult::NoMedia);
    reset(); d.ready = 2; assert(stage() == StagingResult::NoMedia);
    reset(); d.version = 2; assert(stage() == StagingResult::NoMedia);
    reset(); ++d.size; assert(stage() == StagingResult::NoMedia);
    reset(); --d.size; assert(stage() == StagingResult::SizeMismatch);
    reset(); d.path[0] = 'x'; assert(stage() == StagingResult::OpenFailed);
    reset(); d.disconnect_after = 4096;
    assert(stage() == StagingResult::Disconnected);
    assert(destination[4095] == source[4095] && destination[4096] == 0);
    reset(); d.disconnect_after = source.size();
    assert(stage() == StagingResult::CloseFailed);
    reset();
    BackedImageReader reader(d, source.data(), source.size());
    assert(reader.Open("image.bin") && !reader.Open("image.bin"));
    std::size_t count = 99;
    assert(!reader.Read(destination.data(), source.size() + 1, count) && count == 0);
    ++d.generation;
    assert(!reader.Read(destination.data(), 1, count) && count == 0);
    assert(!reader.Close());
    reset();
    BackedImageReader changed(d, source.data(), source.size());
    assert(changed.Open("image.bin"));
    --d.size;
    assert(!changed.Read(destination.data(), 1, count) && !changed.Close());
    reset();
    d.disconnect_after = 1;
    BackedImageReader idle(d, source.data(), source.size());
    idle.Pump();
    assert(d.ready == 1);
    d.disconnect_after = 0;
    idle.Pump();
    assert(d.ready == 1);
    reset();
    BackedImageReader null(d, nullptr, source.size());
    assert(!null.Ready());
}
