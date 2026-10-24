// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include "app/Settings.h"
#include "ui/screens/ListScreen.h"

namespace lgremote {

// First-run checklist for one link mode. Each step shows its live state on the right
// and an explanation at the bottom; enter performs the step. The last row starts the
// remote and only works once every required step is done.
class SetupScreen : public ListScreen {
public:
    SetupScreen(App& app, LinkMode mode);

    void onEnter() override;

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

    LinkMode mode_;
    std::vector<Step> steps_;
    bool tvPrepared_ = false;  // user confirmed the TV-side settings (cannot be detected)
};

}  // namespace lgremote
