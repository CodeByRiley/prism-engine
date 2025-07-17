#pragma once

#include <memory>
#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include <engine/core/networking/NetworkManager.h>

// Forward declarations to avoid full ImGui include in header
struct ImGuiIO;

class NetworkUI {
public:
    NetworkUI();
    virtual ~NetworkUI();
    
    // Initialization and cleanup
    bool Initialize(GLFWwindow* window);
    void Shutdown();
    
    // Rendering
    void Render();
    void RenderContent();  // Renders content without frame management
    
    // State management
    void SetVisible(bool visible) { m_showNetworkWindow = visible; }
    bool IsVisible() const { return m_showNetworkWindow; }
    void ToggleVisibility() { m_showNetworkWindow = !m_showNetworkWindow; }
    
    // Get initialization status
    bool IsInitialized() const { return m_initialized; }
    
    // Network state callbacks (optional - for external monitoring)
    void SetConnectionStateCallback(std::function<void(bool isServer, bool isConnected)> callback) {
        m_connectionStateCallback = callback;
    }
    
private:
    // Internal rendering methods
    void DrawServerPanel();
    void DrawClientPanel(); 
    void DrawConnectionStatus();
    void DrawNetworkStats();
    void DrawConnectedPeers();
    void DrawChatPanel();
    void DrawNetworkLog();
    
    // Helper methods
    void HandleNetworkEvent(const NetworkEvent& event);
    void SendChatMessage();
    void StartServer();
    void ConnectToServer();
    void Disconnect();
    void UpdateNetworkState();
    
    // State variables
    bool m_showNetworkWindow = true;
    bool m_initialized = false;
    GLFWwindow* m_window = nullptr;
    
    // Connection state
    bool m_isNetworkInitialized = false;
    bool m_wasServer = false;
    bool m_wasClient = false;
    
    // UI state
    char m_serverPortBuffer[16] = "7777";
    char m_maxClientsBuffer[16] = "10";
    char m_serverAddressBuffer[256] = "localhost";
    char m_clientPortBuffer[16] = "7777";
    char m_chatMessageBuffer[512] = "";
    char m_playerNameBuffer[64] = "Player";
    
    // Network stats
    struct NetworkStats {
        uint32_t packetsSent = 0;
        uint32_t packetsReceived = 0;
        uint64_t bytesSent = 0;
        uint64_t bytesReceived = 0;
        uint32_t latency = 0;
        size_t peerCount = 0;
        float updateTimer = 0.0f;
    } m_stats;
    
    // Chat and logging
    struct ChatMessage {
        std::string playerName;
        std::string message;
        uint32_t timestamp;
        bool isSystemMessage = false;
    };
    std::vector<ChatMessage> m_chatMessages;
    
    struct LogEntry {
        std::string message;
        uint32_t timestamp;
        int type; // 0 = info, 1 = warning, 2 = error
    };
    std::vector<LogEntry> m_logEntries;
    
    // Connection callback
    std::function<void(bool isServer, bool isConnected)> m_connectionStateCallback;
    
    // UI settings
    struct UISettings {
        bool showStats = true;
        bool showPeers = true;
        bool showChat = true;
        bool showLog = true;
        bool autoScroll = true;
        int maxLogEntries = 100;
        int maxChatMessages = 50;
    } m_settings;
}; 