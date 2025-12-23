#include "UserData.h"

#include <fstream>
#include <array>
#include <limits>
#include <iostream>
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

uint8_t UserData::getCurScore(ScoreType type) {
    return curScores[static_cast<uint8_t>(type)];
}

const std::string& UserData::getUsername() const {
    return username;
}

using json = nlohmann::json;

namespace {
    constexpr size_t kScoreCount =
        static_cast<size_t>(UserData::ScoreType::MAX_SCORE_TYPES);

    // JSON key <-> enum index 매핑 (네 JSON 포맷 기준)
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
    // 1) 파일 열기
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;

    // 2) JSON 파싱
    json j;
    try {
        ifs >> j;
    } catch (...) {
        return false;
    }

    // 3) Snapshot에 먼저 담기 (validate 전 단계)
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

    // 4) (여기서 추가 검증 규칙을 넣을 수 있음)
    // 예: userScore가 curScores 합과 일치해야 한다 등

    // 5) apply (성공했을 때만 실제 멤버 갱신)
    username = std::move(newUsername);
    userTotalScore = newUserScore;
    for (size_t i = 0; i < kScoreCount; ++i) {
        curScores[i] = newCurScores[i];
    }

    return true;
}
