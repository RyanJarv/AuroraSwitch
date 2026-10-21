// Host test of setup ordering and first-error retention; no USB execution.
#include "../support/media_initialization.hpp"
#include <cassert>

int main()
{
    using daisy_development::MediaInitialization;
    const MediaInitialization expected[] = {
        MediaInitialization::UsbFailed, MediaInitialization::FilesystemFailed,
        MediaInitialization::MountFailed, MediaInitialization::Ready};
    for(int failure = 0; failure < 4; ++failure)
    {
        int calls = 0;
        auto step = [&](int index) {
            assert(calls == index);
            ++calls;
            return index != failure;
        };
        const auto result = daisy_development::InitializeMedia(
            [&] { return step(0); }, [&] { return step(1); }, [&] { return step(2); });
        assert(result == expected[failure]);
        assert(calls == (failure < 3 ? failure + 1 : 3));
        // An error stays an error regardless of media/control readiness.
        for(int iteration = 0; iteration < 100; ++iteration)
            assert((result == MediaInitialization::Ready) == (failure == 3));
    }
}
