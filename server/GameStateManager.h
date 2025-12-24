#pragma once

#include "PlayerManager.h"
#include <json/json.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>
#include <queue>
#include <cstdint>

class WebSocketServer;

struct GameAction {
    uint64_t playerId;
    uint64_t actionId;
    uint64_t timestamp;
    std::string actionType;
    Json::Value data; // Action-specific data
    uint64_t clientSequenceNumber;
};

struct GameStateSnapshot {
    uint64_t snapshotId;
    uint64_t timestamp;
    Json::Value state; // Game state data
    std::unordered_map<uint64_t, uint64_t> playerSequenceNumbers;
};

class GameStateManager {
public:
    GameStateManager(PlayerManager* playerManager, WebSocketServer* wsServer);
    ~GameStateManager();
    
    void tick(); // Called every game tick
    void handlePlayerAction(uint64_t playerId, const Json::Value& actionData); // JSON variant
    void broadcastStateUpdates();
    
    void removePlayer(uint64_t playerId);
    
    uint64_t getServerTime() const;
    
    // Rollback/Reconciliation
    void rollbackToSnapshot(uint64_t snapshotId);
    GameStateSnapshot* getSnapshot(uint64_t snapshotId);
    void createSnapshot();
    
private:
    PlayerManager* m_playerManager;
    WebSocketServer* m_wsServer;
    
    // Game state
    Json::Value m_currentState;
    uint64_t m_serverTime;
    uint64_t m_tickCount;
    bool m_stateDirty; // Only broadcast if something changed
    
    // Action queue
    std::queue<GameAction> m_actionQueue;
    std::mutex m_actionQueueMutex;
    
    // Snapshot system for rollback
    std::vector<GameStateSnapshot> m_snapshots;
    std::mutex m_snapshotsMutex;
    static const size_t MAX_SNAPSHOTS = 100;
    
    // Player sequence numbers for reconciliation
    std::unordered_map<uint64_t, uint64_t> m_playerSequenceNumbers;
    std::mutex m_sequenceMutex;
    
    void processActions();
    void applyAction(const GameAction& action);
    bool validateAction(const GameAction& action);
    void simulateTick();
    void cleanupOldSnapshots();
};

