#ifndef USERDATA_H
#define USERDATA_H

#include <cstdint>
#include <string>

class UserData {
public:
    enum class ScoreType : uint8_t {
        SPEED_BUMPS, 
        RAPID_ACCELERATIONS,
        SHARP_TURNS,
        SIGNAL_VIOLATIONS,
        
        MAX_SCORE_TYPES,
    };
private:
    UserData();
    ~UserData() = default;
    UserData(const UserData&) = delete;
    UserData& operator=(const UserData&) = delete;
private:
    uint16_t userTotalScore;
    uint8_t curScores[static_cast<uint8_t>(ScoreType::MAX_SCORE_TYPES)];
    std::string username;
public:
    static UserData& getInstance();
    bool loadFromJsonFile(const std::string& path);
    bool saveToJsonFile(const std::string& path) const;
    uint16_t getUserTotalScore();
    void adjustUserTotalScore(double delta); // clamps to [0, uint16_max]
    uint8_t adjustCurScore(ScoreType type, double delta); // clamps to [0, 255], returns new value
    uint8_t getCurScore(ScoreType type);
    const std::string& getUsername() const;
};
    
#endif // USERDATA_H
