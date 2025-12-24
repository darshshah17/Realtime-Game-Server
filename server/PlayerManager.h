#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

struct Player {
    uint64_t id;
    std::string username;
    bool inMatch;
    std::string currentMatchId;
    uint64_t lastPingTime;
    float latency; // in milliseconds
};

class PlayerManager {
public:
    PlayerManager();
    ~PlayerManager();
    
    void addPlayer(uint64_t playerId);
    void removePlayer(uint64_t playerId);
    bool playerExists(uint64_t playerId) const;
    
    Player* getPlayer(uint64_t playerId);
    const Player* getPlayer(uint64_t playerId) const;
    
    void setPlayerUsername(uint64_t playerId, const std::string& username);
    void setPlayerInMatch(uint64_t playerId, bool inMatch, const std::string& matchId = "");
    void updatePlayerLatency(uint64_t playerId, float latency);
    void updatePlayerPing(uint64_t playerId, uint64_t timestamp);
    
    size_t getPlayerCount() const;
    std::vector<uint64_t> getAllPlayerIds() const;
    
private:
    std::unordered_map<uint64_t, Player> m_players;
    mutable std::mutex m_mutex;
    uint64_t m_nextPlayerId;
};

