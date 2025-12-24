#include "ChatSystem.h"
#include "WebSocketServer.h"
#include <json/json.h>
#include <algorithm>
#include <chrono>
#include <vector>

ChatSystem::ChatSystem(PlayerManager* playerManager, WebSocketServer* wsServer) 
    : m_playerManager(playerManager), m_wsServer(wsServer) {
}

ChatSystem::~ChatSystem() {
}

void ChatSystem::handleMessage(uint64_t playerId, const Json::Value& messageData) {
    std::string message = messageData.get("message", "").asString();
    std::string channel = messageData.get("channel", "global").asString();
    
    if (validateMessage(message)) {
        sendMessage(playerId, message, channel);
    }
}

void ChatSystem::removePlayer(uint64_t playerId) {
    // Player removal is handled by PlayerManager
    // Chat messages remain in history
}

void ChatSystem::sendMessage(uint64_t playerId, const std::string& message, const std::string& channel) {
    const Player* player = m_playerManager->getPlayer(playerId);
    if (!player) {
        return;
    }
    
    ChatMessage chatMsg;
    chatMsg.playerId = playerId;
    chatMsg.username = player->username;
    chatMsg.message = message;
    chatMsg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    chatMsg.channel = channel;
    
    {
        std::lock_guard<std::mutex> lock(m_messagesMutex);
        if (channel == "global") {
            m_globalMessages.push_back(chatMsg);
            
            // Keep only recent messages
            if (m_globalMessages.size() > MAX_MESSAGES_PER_CHANNEL) {
                m_globalMessages.erase(m_globalMessages.begin());
            }
        }
    }
    
    broadcastMessage(chatMsg);
}

std::vector<ChatMessage> ChatSystem::getRecentMessages(const std::string& channel, int count) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_messagesMutex));
    
    std::vector<ChatMessage> result;
    if (channel == "global") {
        int start = std::max(0, static_cast<int>(m_globalMessages.size()) - count);
        result.assign(m_globalMessages.begin() + start, m_globalMessages.end());
    }
    
    return result;
}

void ChatSystem::broadcastMessage(const ChatMessage& chatMsg) {
    // Create JSON response
    Json::Value response;
    response["type"] = "chat_message";
    response["playerId"] = static_cast<Json::UInt64>(chatMsg.playerId);
    response["username"] = chatMsg.username;
    response["message"] = chatMsg.message;
    response["timestamp"] = static_cast<Json::UInt64>(chatMsg.timestamp);
    response["channel"] = chatMsg.channel;
    
    if (m_wsServer) {
        if (chatMsg.channel == "global") {
            m_wsServer->broadcast(response.toStyledString());
        } else {
            m_wsServer->broadcastToRoom(chatMsg.channel, response.toStyledString());
        }
    }
}

bool ChatSystem::validateMessage(const std::string& message) const {
    // Basic validation
    if (message.empty() || message.length() > 500) {
        return false;
    }
    
    // Check for profanity, spam, etc. (simplified)
    // In production, use a proper content moderation system
    
    return true;
}

