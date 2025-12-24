#pragma once

#include "PlayerManager.h"
#include <json/json.h>
#include <queue>
#include <vector>
#include <string>
#include <mutex>
#include <unordered_map>
#include <cstdint>

class WebSocketServer;

struct MatchmakingRequest {
    uint64_t playerId;
    std::string gameMode;
    int minPlayers;
    int maxPlayers;
    uint64_t timestamp;
};

struct Match {
    std::string matchId;
    std::vector<uint64_t> players;
    std::string gameMode;
    uint64_t createdAt;
    bool isActive;
};

class MatchmakingSystem {
public:
    MatchmakingSystem(PlayerManager* playerManager, WebSocketServer* wsServer);
    ~MatchmakingSystem();
    
    void queuePlayer(uint64_t playerId, const std::string& gameMode, int minPlayers = 2, int maxPlayers = 4);
    void queuePlayer(uint64_t playerId, const Json::Value& requestData); // JSON variant
    void removePlayer(uint64_t playerId);
    
    void process(); // Called every tick to process matchmaking
    
    Match* getMatch(const std::string& matchId);
    Match* getPlayerMatch(uint64_t playerId);
    
    void endMatch(const std::string& matchId);
    
private:
    PlayerManager* m_playerManager;
    WebSocketServer* m_wsServer;
    std::queue<MatchmakingRequest> m_queue;
    std::mutex m_queueMutex;
    
    std::unordered_map<std::string, Match> m_matches;
    std::unordered_map<uint64_t, std::string> m_playerToMatch;
    std::mutex m_matchesMutex;
    
    std::string generateMatchId();
    bool canFormMatch(const MatchmakingRequest& request, const std::vector<MatchmakingRequest>& candidates);
    void createMatch(const std::vector<uint64_t>& players, const std::string& gameMode);
    void notifyMatchCreated(const Match& match);
};

