#include "WebSocketServer.h"
#include <libwebsockets.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <deque>
#include <string>
#include <cstring>
#include <new> // For placement new

// Use the struct from the class
using PerSessionData = WebSocketServer::PerSessionData;

// Global server instance (libwebsockets limitation)
static WebSocketServer* g_serverInstance = nullptr;

static void ensure_session_initialized(PerSessionData* pss, const char* context) {
    if (pss && !pss->initialized) {
        new (pss) PerSessionData();
    }
}

static int callback_websocket(struct lws *wsi, enum lws_callback_reasons reason,
                              void *user, void *in, size_t len) {
    PerSessionData* pss = (PerSessionData*)user;
    
    switch (reason) {
        case LWS_CALLBACK_WSI_CREATE: {
            if (pss) ensure_session_initialized(pss, "WSI_CREATE");
            break;
        }

        case LWS_CALLBACK_WSI_DESTROY: {
            if (pss && pss->initialized) {
                pss->~PerSessionData();
                pss->initialized = false;
            }
            break;
        }
        
        case LWS_CALLBACK_HTTP: {
            if (pss) ensure_session_initialized(pss, "HTTP");
            return 0;
        }
        
        case LWS_CALLBACK_ESTABLISHED: {
            if (!pss) return -1;
            ensure_session_initialized(pss, "ESTABLISHED");
            
            if (g_serverInstance) {
                g_serverInstance->onConnect(wsi);
            }
            break;
        }
        
        case LWS_CALLBACK_RECEIVE: {
            if (!pss) return -1;
            ensure_session_initialized(pss, "RECEIVE");
            
            if (g_serverInstance && in && len > 0) {
                std::string message((char*)in, len);
                g_serverInstance->onMessage(wsi, message);
            }
            break;
        }
        
        case LWS_CALLBACK_SERVER_WRITEABLE: {
            if (!pss || !pss->initialized) break;
            
            std::string message;
            bool hasMessage = false;
            
            try {
                std::lock_guard<std::mutex> lock(pss->queueMutex);
                if (!pss->writeQueue.empty()) {
                    message = pss->writeQueue.front();
                    pss->writeQueue.pop_front();
                    hasMessage = true;
                }
            } catch (...) { return -1; }
            
            if (hasMessage) {
                unsigned char* buf = new unsigned char[LWS_PRE + message.length()];
                memcpy(&buf[LWS_PRE], message.c_str(), message.length());
                int ret = lws_write(wsi, &buf[LWS_PRE], message.length(), LWS_WRITE_TEXT);
                delete[] buf;
                
                if (ret < 0) return -1;
                
                lws_callback_on_writable(wsi); // Check if more needed
            }
            break;
        }
        
        case LWS_CALLBACK_CLOSED: {
            if (g_serverInstance && pss) {
                g_serverInstance->onDisconnect(wsi);
            }
            break;
        }
        
        default:
            break;
    }
    
    return 0;
}

// Protocol list
static struct lws_protocols protocols[] = {
    {
        "game-websocket",
        callback_websocket,
        sizeof(PerSessionData),
        4096,
    },
    { nullptr, nullptr, 0, 0 }
};

WebSocketServer::WebSocketServer(int port) 
    : m_port(port), m_running(false), context(nullptr), m_nextClientId(1) {
    g_serverInstance = this;
}

WebSocketServer::~WebSocketServer() {
    stop();
    g_serverInstance = nullptr;
}

void WebSocketServer::run() {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = m_port;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;
    info.pt_serv_buf_size = 4096;
    
    context = lws_create_context(&info);
    if (!context) {
        std::cerr << "Failed to create libwebsockets context" << std::endl;
        return;
    }
    
    std::cout << "[WebSocketServer] Server started on port " << m_port << std::endl;
    m_running = true;
    
    while (m_running && context) {
        lws_service(context, 1); // 1ms poll for low latency
    }
    
    lws_context_destroy(context);
    context = nullptr;
}

void WebSocketServer::stop() {
    m_running = false;
}

void WebSocketServer::setOnConnect(ConnectCallback callback) {
    m_onConnect = callback;
}

void WebSocketServer::setOnDisconnect(DisconnectCallback callback) {
    m_onDisconnect = callback;
}

void WebSocketServer::setOnMessage(MessageCallback callback) {
    m_onMessage = callback;
}

void WebSocketServer::onConnect(struct lws* wsi) {
    std::lock_guard<std::recursive_mutex> lock(m_clientMapMutex);
    uint64_t id = m_nextClientId++;
    
    m_wsiToId[wsi] = id;
    m_idToWsi[id] = wsi;
    
    // Set ID in session data
    PerSessionData* pss = (PerSessionData*)lws_wsi_user(wsi);
    if (pss) pss->clientId = id;
    
    std::cout << "[WebSocket] Client " << id << " connected" << std::endl;
    
    if (m_onConnect) m_onConnect(id);
}

void WebSocketServer::onDisconnect(struct lws* wsi) {
    std::lock_guard<std::recursive_mutex> lock(m_clientMapMutex);
    auto it = m_wsiToId.find(wsi);
    if (it != m_wsiToId.end()) {
        uint64_t id = it->second;
        m_wsiToId.erase(it);
        m_idToWsi.erase(id);
        
        if (m_onDisconnect) m_onDisconnect(id);
    }
}

void WebSocketServer::onMessage(struct lws* wsi, const std::string& message) {
    std::lock_guard<std::recursive_mutex> lock(m_clientMapMutex);
    auto it = m_wsiToId.find(wsi);
    if (it != m_wsiToId.end()) {
        if (m_onMessage) m_onMessage(it->second, message);
    }
}

void WebSocketServer::send(uint64_t clientId, const std::string& message) {
    std::lock_guard<std::recursive_mutex> lock(m_clientMapMutex);
    auto it = m_idToWsi.find(clientId);
    if (it != m_idToWsi.end()) {
        struct lws* wsi = it->second;
        PerSessionData* pss = (PerSessionData*)lws_wsi_user(wsi);
        if (pss && pss->initialized) {
            {
                std::lock_guard<std::mutex> lock(pss->queueMutex);
                pss->writeQueue.push_back(message);
            }
            lws_callback_on_writable(wsi);
        }
    }
}

void WebSocketServer::broadcast(const std::string& message) {
    std::lock_guard<std::recursive_mutex> lock(m_clientMapMutex);
    for (auto& pair : m_idToWsi) {
        struct lws* wsi = pair.second;
        PerSessionData* pss = (PerSessionData*)lws_wsi_user(wsi);
        if (pss && pss->initialized) {
            {
                std::lock_guard<std::mutex> lock(pss->queueMutex);
                pss->writeQueue.push_back(message);
            }
            lws_callback_on_writable(wsi);
        }
    }
}

void WebSocketServer::broadcastToRoom(const std::string& roomId, const std::string& message) {
    std::lock_guard<std::recursive_mutex> lock(m_clientMapMutex);
    for (auto& pair : m_idToWsi) {
        struct lws* wsi = pair.second;
        PerSessionData* pss = (PerSessionData*)lws_wsi_user(wsi);
        if (pss && pss->initialized && pss->roomId == roomId) {
            {
                std::lock_guard<std::mutex> lock(pss->queueMutex);
                pss->writeQueue.push_back(message);
            }
            lws_callback_on_writable(wsi);
        }
    }
}

void WebSocketServer::setClientRoom(uint64_t clientId, const std::string& roomId) {
    std::lock_guard<std::recursive_mutex> lock(m_clientMapMutex);
    auto it = m_idToWsi.find(clientId);
    if (it != m_idToWsi.end()) {
        struct lws* wsi = it->second;
        PerSessionData* pss = (PerSessionData*)lws_wsi_user(wsi);
        if (pss) {
            pss->roomId = roomId;
        }
    }
}

uint64_t WebSocketServer::getClientId(struct lws* wsi) const {
    std::lock_guard<std::recursive_mutex> lock(m_clientMapMutex);
    auto it = m_wsiToId.find(wsi);
    return (it != m_wsiToId.end()) ? it->second : 0;
}
