#pragma once
#include <string>

enum class ClawdState {
    kIdle, kThinking, kTyping, kBuilding, kError, kHappy,
    kSleeping, kSubagent, kNotification, kSweeping, kCarrying, kSubagentMulti
};

ClawdState clawd_state_from_string(const std::string &s);
const char *clawd_state_to_string(ClawdState s);
