#include "GameStateManager.h"
#include "WebSocketServer.h"
#include <json/json.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <random>
#include <iostream>

GameStateManager::GameStateManager(PlayerManager* playerManager, WebSocketServer* wsServer) 
    : m_playerManager(playerManager), m_wsServer(wsServer), m_serverTime(0), m_tickCount(0), m_stateDirty(false) {
    m_currentState["players"] = Json::Value(Json::objectValue);
    m_currentState["entities"] = Json::Value(Json::arrayValue);
    m_currentState["worldState"] = Json::Value(Json::objectValue);
}

GameStateManager::~GameStateManager() {
}

void GameStateManager::tick() {
    m_tickCount++;
    m_serverTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    // Reset dirty flag at start of tick
    m_stateDirty = false;
    
    processActions();
    simulateTick();
    
    // Always broadcast if there are actions, otherwise skip to save bandwidth
    // In a real game, you might want a heartbeat (e.g. every 60 ticks) even if nothing changes
    if (m_stateDirty || m_tickCount % 60 == 0) {
        broadcastStateUpdates();
    }
    
    // Create snapshot periodically (every 10 ticks)
    if (m_tickCount % 10 == 0) {
        createSnapshot();
    }
    
    cleanupOldSnapshots();
}

void GameStateManager::handlePlayerAction(uint64_t playerId, const Json::Value& actionData) {
    GameAction action;
    action.playerId = playerId;
    action.actionId = actionData.get("actionId", 0).asUInt64();
    action.timestamp = actionData.get("timestamp", static_cast<Json::UInt64>(m_serverTime)).asUInt64();
    action.actionType = actionData.get("actionType", "").asString();
    action.data = actionData.get("data", Json::Value());
    action.clientSequenceNumber = actionData.get("sequenceNumber", 0).asUInt64();
    
    // For spawn requests, we don't need strict validation on sequence
    if (action.actionType == "spawn" || validateAction(action)) {
        std::lock_guard<std::mutex> lock(m_actionQueueMutex);
        m_actionQueue.push(action);
        std::cout << "[GameState] Queued action: " << action.actionType << " for player " << playerId << std::endl;
    } else {
        std::cout << "[GameState] REJECTED action: " << action.actionType << " for player " << playerId << std::endl;
    }
}

void GameStateManager::broadcastStateUpdates() {
    // Create state update message
    Json::Value update;
    update["type"] = "state_update";
    update["serverTime"] = static_cast<Json::UInt64>(m_serverTime);
    update["tick"] = static_cast<Json::UInt64>(m_tickCount);
    update["state"] = m_currentState;
    
    if (m_wsServer) {
        m_wsServer->broadcast(update.toStyledString());
    }
}

void GameStateManager::removePlayer(uint64_t playerId) {
    std::lock_guard<std::mutex> lock(m_sequenceMutex);
    m_playerSequenceNumbers.erase(playerId);
    
    // Remove from game state
    m_currentState["players"].removeMember(std::to_string(playerId));
}

uint64_t GameStateManager::getServerTime() const {
    return m_serverTime;
}

void GameStateManager::processActions() {
    std::lock_guard<std::mutex> lock(m_actionQueueMutex);
    
    while (!m_actionQueue.empty()) {
        GameAction action = m_actionQueue.front();
        m_actionQueue.pop();
        
        applyAction(action);
    }
}

void GameStateManager::applyAction(const GameAction& action) {
    const Player* player = m_playerManager->getPlayer(action.playerId);
    if (!player) {
        return;
    }
    
    // Spawn Action - New 8x8 Grid Logic
    if (action.actionType == "spawn") {
        std::string playerKey = std::to_string(action.playerId);
        
        // Random Position 0-7
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 7);
        
        // Find a free spot (simple approach, try 10 times)
        int x = dis(gen);
        int y = dis(gen);
        
        // Save to state
        m_currentState["players"][playerKey] = Json::Value(Json::objectValue);
        m_currentState["players"][playerKey]["x"] = x;
        m_currentState["players"][playerKey]["y"] = y;
        m_stateDirty = true;
        
        std::cout << "Player " << action.playerId << " spawned at (" << x << ", " << y << ")" << std::endl;
        return;
    }

    // Move Action - New 8x8 Grid Logic
    if (action.actionType == "move") {
        std::string playerKey = std::to_string(action.playerId);
        
        // Only allow move if player exists in state (spawned)
        if (m_currentState["players"].isMember(playerKey)) {
            int currentX = m_currentState["players"][playerKey]["x"].asInt();
            int currentY = m_currentState["players"][playerKey]["y"].asInt();
            
            int dx = action.data.get("dx", 0).asInt();
            int dy = action.data.get("dy", 0).asInt();
            
            // Calculate new pos
            int newX = currentX + dx;
            int newY = currentY + dy;
            
            // Clamp to grid 0-7
            if (newX >= 0 && newX <= 7 && newY >= 0 && newY <= 7) {
                m_currentState["players"][playerKey]["x"] = newX;
                m_currentState["players"][playerKey]["y"] = newY;
                m_stateDirty = true;
            }
        }
        return;
    }
    
    // Shoot Action
    if (action.actionType == "shoot") {
        // Create projectile entity (simplified for now)
        Json::Value projectile(Json::objectValue);
        projectile["id"] = static_cast<Json::UInt64>(m_tickCount * 1000 + action.actionId);
        projectile["type"] = "projectile";
        projectile["ownerId"] = static_cast<Json::UInt64>(action.playerId);
        // ... projectile logic could be added here
        
        // For now, just log it
        // std::cout << "Player " << action.playerId << " shot!" << std::endl;
    }
}

bool GameStateManager::validateAction(const GameAction& action) {
    const Player* player = m_playerManager->getPlayer(action.playerId);
    if (!player) {
        return false;
    }
    
    // Check timestamp
    // NOTE: Disabled strict timestamp check for demo because client uses Date.now() (wall clock)
    // and server uses steady_clock (uptime), causing mismatch.
    // In production, sync clocks or use server-authoritative timestamps.
    /*
    int64_t timeDiff = static_cast<int64_t>(m_serverTime) - static_cast<int64_t>(action.timestamp);
    if (timeDiff > 5000 || timeDiff < -100) { 
        return false;
    }
    */
    
    if (action.actionType.empty()) {
        return false;
    }
    
    return true;
}

void GameStateManager::simulateTick() {
    // Game physics simulation (projectiles, etc.) can go here
}

void GameStateManager::createSnapshot() {
    std::lock_guard<std::mutex> lock(m_snapshotsMutex);
    
    GameStateSnapshot snapshot;
    snapshot.snapshotId = m_tickCount;
    snapshot.timestamp = m_serverTime;
    snapshot.state = m_currentState;
    snapshot.playerSequenceNumbers = m_playerSequenceNumbers;
    
    m_snapshots.push_back(snapshot);
    
    if (m_snapshots.size() > MAX_SNAPSHOTS) {
        m_snapshots.erase(m_snapshots.begin());
    }
}

void GameStateManager::rollbackToSnapshot(uint64_t snapshotId) {
    std::lock_guard<std::mutex> lock(m_snapshotsMutex);
    auto it = std::find_if(m_snapshots.begin(), m_snapshots.end(),
        [snapshotId](const GameStateSnapshot& s) { return s.snapshotId == snapshotId; });
    if (it != m_snapshots.end()) {
        m_currentState = it->state;
        m_playerSequenceNumbers = it->playerSequenceNumbers;
    }
}

GameStateSnapshot* GameStateManager::getSnapshot(uint64_t snapshotId) {
    std::lock_guard<std::mutex> lock(m_snapshotsMutex);
    auto it = std::find_if(m_snapshots.begin(), m_snapshots.end(),
        [snapshotId](const GameStateSnapshot& s) { return s.snapshotId == snapshotId; });
    if (it != m_snapshots.end()) {
        return &(*it);
    }
    return nullptr;
}

void GameStateManager::cleanupOldSnapshots() {
    std::lock_guard<std::mutex> lock(m_snapshotsMutex);
    uint64_t cutoffTime = m_serverTime - 5000;
    m_snapshots.erase(
        std::remove_if(m_snapshots.begin(), m_snapshots.end(),
            [cutoffTime](const GameStateSnapshot& s) { return s.timestamp < cutoffTime; }),
        m_snapshots.end());
}
