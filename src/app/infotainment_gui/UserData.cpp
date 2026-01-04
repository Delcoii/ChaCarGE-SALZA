#include "UserData.h"

#include <fstream>
#include <array>
#include <limits>
#include <iostream>
#include <cmath>
#include <nlohmann/json.hpp>

UserData::UserData() : userTotalScore(0) {
    for (auto& score : curScores) {
        score = 0;
    }
}

UserData& UserData::getInstance() {
    static UserData instance;
    return instance;
}

uint16_t UserData::getUserTotalScore() {
    return userTotalScore;
}

void UserData::adjustUserTotalScore(double delta) {
    const auto deltaInt = static_cast<int>(std::llround(delta));
    int newVal = static_cast<int>(userTotalScore) + deltaInt;
    if (newVal < 0) newVal = 0;
    if (newVal > std::numeric_limits<uint16_t>::max()) newVal = std::numeric_limits<uint16_t>::max();
    userTotalScore = static_cast<uint16_t>(newVal);
}

uint8_t UserData::getCurScore(ScoreType type) {
    return curScores[static_cast<uint8_t>(type)];
}

const std::string& UserData::getUsername() const {
    return username;
}

uint8_t UserData::adjustCurScore(ScoreType type, double delta) {
    const auto idx = static_cast<uint8_t>(type);
    const auto deltaInt = static_cast<int>(std::llround(delta));
    int newVal = static_cast<int>(curScores[idx]) + deltaInt;
    if (newVal < 0) newVal = 0;
    if (newVal > std::numeric_limits<uint8_t>::max()) newVal = std::numeric_limits<uint8_t>::max();
    curScores[idx] = static_cast<uint8_t>(newVal);
    return curScores[idx];
}

using json = nlohmann::json;

namespace {
    constexpr size_t kScoreCount =
        static_cast<size_t>(UserData::ScoreType::MAX_SCORE_TYPES);

    // Map JSON keys to enum indices (per the existing JSON format)
    constexpr const char* kScoreKeys[kScoreCount] = {
        "SPEED_BUMPS",
        "RAPID_ACCELERATIONS",
        "SHARP_TURNS",
        "SIGNAL_VIOLATIONS"
    };
}

bool UserData::saveToJsonFile(const std::string& path) const {
    json j;
    j["username"] = username;
    j["userScore"] = userTotalScore;

    json scoresObj = json::object();
    for (size_t i = 0; i < kScoreCount; ++i) {
        scoresObj[kScoreKeys[i]] = curScores[i];
    }
    j["curScores"] = std::move(scoresObj);

    std::ofstream ofs(path);
    if (!ofs.is_open()) return false;
    ofs << j.dump(4);

    // Log current scores to terminal
    std::cout << "[UserData] Saving curScores -> ";
    for (size_t i = 0; i < kScoreCount; ++i) {
        std::cout << kScoreKeys[i] << ":" << static_cast<int>(curScores[i]);
        if (i + 1 < kScoreCount) std::cout << ", ";
    }
    std::cout << std::endl;

    return true;
}

bool UserData::loadFromJsonFile(const std::string& path)
{
    // 1) Open file
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;

    // 2) Parse JSON
    json j;
    try {
        ifs >> j;
    } catch (...) {
        return false;
    }

    // 3) Stage values in a snapshot (before validation)
    uint16_t newUserScore = 0;
    std::array<uint8_t, kScoreCount> newCurScores{};
    newCurScores.fill(0);
    std::string newUsername;

    try {
        // username
        if (!j.contains("username") || !j["username"].is_string())
            return false;
        newUsername = j["username"].get<std::string>();

        // userScore
        if (!j.contains("userScore") || !j["userScore"].is_number_integer())
            return false;

        const int us = j["userScore"].get<int>();
        if (us < 0 || us > std::numeric_limits<uint16_t>::max())
            return false;
        newUserScore = static_cast<uint16_t>(us);

        // curScores
        if (!j.contains("curScores") || !j["curScores"].is_object())
            return false;

        const auto& scores = j["curScores"];
        for (size_t i = 0; i < kScoreCount; ++i) {
            const char* key = kScoreKeys[i];
            if (!scores.contains(key) || !scores[key].is_number_integer())
                return false;

            const int v = scores[key].get<int>();
            if (v < 0 || v > std::numeric_limits<uint8_t>::max())
                return false;

            newCurScores[i] = static_cast<uint8_t>(v);
        }
    } catch (...) {
        return false;
    }

    // 4) (Insert additional validation rules here if needed)
    // e.g., require userScore to match the sum of curScores

    // 5) Apply (update members only on success)
    username = std::move(newUsername);
    userTotalScore = newUserScore;
    for (size_t i = 0; i < kScoreCount; ++i) {
        curScores[i] = newCurScores[i];
    }

    return true;
}
