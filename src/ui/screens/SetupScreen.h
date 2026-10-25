// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include "app/Settings.h"
#include "ui/screens/ListScreen.h"

namespace lgremote {

// First-run checklist for one link mode. Each step shows its live state on the right
// and an explanation at the bottom; enter performs the step. Later steps stay locked
// until the earlier ones are done. When the last required step completes, the remote
// opens by itself with a short "connected" pop-up.
class SetupScreen : public ListScreen {
public:
    SetupScreen(App& app, LinkMode mode);

    void onEnter() override;
    void tick() override;

private:
    struct Step {
        const char* label;
        std::function<bool()> isDone;
        bool required;
    };

    void build();
    void addStep(const char* label, const char* description, std::function<bool()> isDone,
                 std::function<void()> action, std::function<String()> value = nullptr, bool required = true);
    void addStartRow();
    bool allDone() const;
    // Index of the first unfinished required step before `index`, or -1.
    int firstOpenStepBefore(size_t index) const;
    String lockedReason(int openStep) const;
    void selectFirstOpenStep();
    void finish();

    LinkMode mode_;
    std::vector<Step> steps_;
    bool tvPrepared_ = false;      // user confirmed the TV-side settings (cannot be detected)
    bool irTested_ = false;        // optional IR test answered with "yes"
    bool completeAtStart_ = false;  // opened only to repeat a finished setup: no auto-start
    bool warned_ = false;           // the "something to do" blink is shown once
};

}  // namespace lgremote
