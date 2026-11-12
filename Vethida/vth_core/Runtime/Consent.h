#pragma once

// The first-run crash report choice.
namespace vth::consent
{
    // An answer to older wording counts as unanswered, so the person is asked again.
    bool decided();

    void record(bool crashOptIn);
}
