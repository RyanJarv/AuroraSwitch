// Synthetic ordering test; does not implement or simulate hardware registers.
#include "handoff_sequence.hpp"
#include <cassert>
#include <vector>

// Records order; terminal methods return only so the host can inspect failure paths.
struct Platform
{
    int validations = 0;
    bool initial_valid = true, final_valid = true, unmount = true, install = true;
    std::vector<int> steps;
    bool Validate() { steps.push_back(0); return ++validations == 1 ? initial_valid : final_valid; }
    bool Unmount() { steps.push_back(1); return unmount; }
    void StopMedia() { steps.push_back(2); }
    void MaskInterrupts() { steps.push_back(3); }
    void ResetBusMasters() { steps.push_back(4); }
    void DisableRuntime() { steps.push_back(5); }
    bool InstallTrampoline() { steps.push_back(6); return install; }
    void Fatal() { steps.push_back(7); }
    void Jump() { steps.push_back(8); }
};

int main()
{
    Platform good;
    assert(aurora_selector::PrepareAndJump(good));
    assert((good.steps == std::vector<int>{0, 1, 2, 3, 4, 5, 0, 6, 8}));
    Platform stale;
    stale.initial_valid = false;
    assert(!aurora_selector::PrepareAndJump(stale));
    assert((stale.steps == std::vector<int>{0}));
    Platform media;
    media.unmount = false;
    assert(!aurora_selector::PrepareAndJump(media));
    assert((media.steps == std::vector<int>{0, 1}));
    Platform mutated;
    mutated.final_valid = false;
    assert(!aurora_selector::PrepareAndJump(mutated));
    assert((mutated.steps == std::vector<int>{0, 1, 2, 3, 4, 5, 0, 7}));
    Platform missing_blob;
    missing_blob.install = false;
    assert(!aurora_selector::PrepareAndJump(missing_blob));
    assert((missing_blob.steps == std::vector<int>{0, 1, 2, 3, 4, 5, 0, 6, 7}));
}
