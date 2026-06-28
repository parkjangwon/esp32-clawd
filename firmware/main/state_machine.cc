#include "state_machine.h"
#include <esp_log.h>

static const char *TAG = "state_machine";

struct StateEntry {
    const char *name;
    ClawdState state;
};

static const StateEntry kStateMap[] = {
    {"idle", ClawdState::kIdle},
    {"thinking", ClawdState::kThinking},
    {"typing", ClawdState::kTyping},
    {"building", ClawdState::kBuilding},
    {"error", ClawdState::kError},
    {"happy", ClawdState::kHappy},
    {"sleeping", ClawdState::kSleeping},
    {"subagent", ClawdState::kSubagent},
    {"notification", ClawdState::kNotification},
    {"sweeping", ClawdState::kSweeping},
    {"carrying", ClawdState::kCarrying},
    {"subagent_multi", ClawdState::kSubagentMulti},
};

ClawdState clawd_state_from_string(const std::string &s) {
    for (const auto &entry : kStateMap) {
        if (s == entry.name) return entry.state;
    }
    ESP_LOGW(TAG, "unknown state string: %s", s.c_str());
    return ClawdState::kIdle;
}

const char *clawd_state_to_string(ClawdState s) {
    for (const auto &entry : kStateMap) {
        if (s == entry.state) return entry.name;
    }
    return "idle";
}
