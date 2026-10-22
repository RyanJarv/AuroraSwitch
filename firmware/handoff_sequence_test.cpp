#include "handoff_sequence.hpp"
#include <cassert>
#include <vector>
struct Platform {
    int validations = 0;
    bool initial = true, final = true, install = true;
    std::vector<int> steps;
    bool Validate() {
        steps.push_back(0);
        return ++validations == 1 ? initial : final;
    }
    void MaskInterrupts() { steps.push_back(1); }
    void ResetBusMasters() { steps.push_back(2); }
    void DisableRuntime() { steps.push_back(3); }
    bool InstallTrampoline() {
        steps.push_back(4);
        return install;
    }
    void Fatal() { steps.push_back(5); }
    void Jump() { steps.push_back(6); }
};
int main() {
    Platform good;
    assert(aurora_selector::PrepareAndJump(good));
    assert((good.steps == std::vector<int>{0, 1, 2, 3, 0, 4, 6}));
    Platform stale;
    stale.initial = false;
    assert(!aurora_selector::PrepareAndJump(stale));
    assert((stale.steps == std::vector<int>{0}));
    Platform changed;
    changed.final = false;
    assert(!aurora_selector::PrepareAndJump(changed));
    assert((changed.steps == std::vector<int>{0, 1, 2, 3, 0, 5}));
    Platform bad_jump;
    bad_jump.install = false;
    assert(!aurora_selector::PrepareAndJump(bad_jump));
    assert((bad_jump.steps == std::vector<int>{0, 1, 2, 3, 0, 4, 5}));
}
