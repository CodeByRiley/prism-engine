#include "NetworkUI.h"
#include <engine/utils/Logger.h>
#include <engine/utils/Time.h>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iomanip>

// ImGui includes
#include <imgui.h>

// Forward declaration for game callback
// extern void OnNetworkUIDisconnect(); // This line is removed as per the edit hint.

NetworkUI::NetworkUI() {
    // Initialize with a default player name
    static int playerCounter = 1;
    snprintf(m_playerNameBuffer, sizeof(m_playerNameBuffer), "Player_%d", playerCounter++);
}

NetworkUI::~NetworkUI() {
    Shutdown();
}

bool NetworkUI::Initialize(GLFWwindow* window) {
    if (m_initialized) {
        return true;
    }
    
    m_window = window;
    
    // Initialize networking system
    if (!Network::Initialize()) {
        Logger::Error<NetworkUI>("Failed to initialize networking system", this);
        return false;
    }
    
    m_isNetworkInitialized = true;
    
    // Set up network event callback
    Network::GetManager().SetEventCallback([this](const NetworkEvent& event) {
        HandleNetworkEvent(event);
    });
    
    // Set up packet handlers for chat
    Network::GetManager().RegisterPacketHandler(PacketType::CHAT_MESSAGE,
        [this](const Packet& packet, uint32_t senderID) {
            PacketData::ChatMessage chatData;
            Packet mutablePacket = packet;
            chatData.ReadFrom(mutablePacket);
            
            ChatMessage msg;
            msg.playerName = chatData.playerName;
            msg.message = chatData.message;
            msg.timestamp = enet_time_get();
            msg.isSystemMessage = false;
            
            m_chatMessages.push_back(msg);
            
            // Limit chat history
            if (m_chatMessages.size() > m_settings.maxChatMessages) {
                m_chatMessages.erase(m_chatMessages.begin());
            }
        });
    
    m_initialized = true;
    
    // Add welcome log entry
    LogEntry entry;
    entry.message = "Network UI initialized successfully";
    entry.timestamp = enet_time_get();
    entry.type = 0; // info
    m_logEntries.push_back(entry);
    
    Logger::Info("NetworkUI initialized successfully");
    return true;
}

void NetworkUI::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    if (m_isNetworkInitialized) {
        Network::Shutdown();
        m_isNetworkInitialized = false;
    }
    
    m_initialized = false;
    Logger::Info("NetworkUI shut down");
}

void NetworkUI::Render() {
    if (!m_initialized || !m_showNetworkWindow) {
        return;
    }
    
    // For standalone rendering (backward compatibility)
    // This would typically not be used when integrated with Game's centralized rendering
    UpdateNetworkState();
    RenderContent();
}

void NetworkUI::RenderContent() {
    if (!m_initialized || !m_showNetworkWindow) {
        return;
    }
    
    UpdateNetworkState();
    
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Network Manager", &m_showNetworkWindow)) {
        
        // Tab bar for different sections
        if (ImGui::BeginTabBar("NetworkTabs")) {
            
            // Connection tab
            if (ImGui::BeginTabItem("Connection")) {
                DrawConnectionStatus();
                ImGui::Separator();
                
                // Show appropriate panel based on current state
                auto& manager = Network::GetManager();
                if (!manager.IsServer() && !manager.IsClient()) {
                    // Not connected - show both server and client options
                    if (ImGui::BeginChild("ServerPanel", ImVec2(0, 150), true)) {
                        DrawServerPanel();
                    }
                    ImGui::EndChild();
                    
                    if (ImGui::BeginChild("ClientPanel", ImVec2(0, 150), true)) {
                        DrawClientPanel();
                    }
                    ImGui::EndChild();
                } else {
                    // Connected - show disconnect option
                    if (ImGui::Button("Disconnect", ImVec2(120, 30))) {
                        Disconnect();
                    }
                    
                    if (manager.IsServer()) {
                        ImGui::SameLine();
                        ImGui::Text("Running as SERVER");
                    } else if (manager.IsClient()) {
                        ImGui::SameLine();
                        ImGui::Text("Connected as CLIENT");
                    }
                }
                
                ImGui::EndTabItem();
            }
            
            // Statistics tab
            if (ImGui::BeginTabItem("Statistics")) {
                DrawNetworkStats();
                ImGui::EndTabItem();
            }
            
            // Peers tab (only show if we have connections)
            auto& manager = Network::GetManager();
            if (manager.GetPeerCount() > 0 && ImGui::BeginTabItem("Peers")) {
                DrawConnectedPeers();
                ImGui::EndTabItem();
            }
            
            // Chat tab
            if (ImGui::BeginTabItem("Chat")) {
                DrawChatPanel();
                ImGui::EndTabItem();
            }
            
            // Log tab
            if (ImGui::BeginTabItem("Log")) {
                DrawNetworkLog();
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void NetworkUI::DrawServerPanel() {
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "START SERVER");
    
    ImGui::Text("Port:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::InputText("##ServerPort", m_serverPortBuffer, sizeof(m_serverPortBuffer), ImGuiInputTextFlags_CharsDecimal);
    
    ImGui::Text("Max Clients:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::InputText("##MaxClients", m_maxClientsBuffer, sizeof(m_maxClientsBuffer), ImGuiInputTextFlags_CharsDecimal);
    
    if (ImGui::Button("Start Server", ImVec2(120, 30))) {
        StartServer();
    }
}

void NetworkUI::DrawClientPanel() {
    ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "CONNECT TO SERVER");
    
    ImGui::Text("Address:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("##ServerAddress", m_serverAddressBuffer, sizeof(m_serverAddressBuffer));
    
    ImGui::Text("Port:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::InputText("##ClientPort", m_clientPortBuffer, sizeof(m_clientPortBuffer), ImGuiInputTextFlags_CharsDecimal);
    
    ImGui::Text("Player Name:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    ImGui::InputText("##PlayerName", m_playerNameBuffer, sizeof(m_playerNameBuffer));
    
    if (ImGui::Button("Connect", ImVec2(120, 30))) {
        ConnectToServer();
    }
}

void NetworkUI::DrawConnectionStatus() {
    auto& manager = Network::GetManager();
    
    // Connection status indicator
    if (manager.IsServer()) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "● SERVER RUNNING");
        ImGui::SameLine();
        ImGui::Text("- %zu clients connected", manager.GetPeerCount());
    } else if (manager.IsClient()) {
        ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "● CONNECTED TO SERVER");
        if (manager.IsConnectedToServer()) {
            ImGui::SameLine();
            ImGui::Text("- Latency: %ums", manager.GetLatency(0));
        }
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "● DISCONNECTED");
    }
}

void NetworkUI::DrawNetworkStats() {
    auto& manager = Network::GetManager();
    
    // Real-time stats
    if (ImGui::BeginChild("StatsChild", ImVec2(0, 0), false)) {
        ImGui::Columns(2, "StatsColumns", false);
        
        // Left column - Basic stats
        ImGui::Text("Connection Statistics");
        ImGui::Separator();
        
        ImGui::Text("Packets Sent:");
        ImGui::NextColumn();
        ImGui::Text("%u", manager.GetPacketsSent());
        ImGui::NextColumn();
        
        ImGui::Text("Packets Received:");
        ImGui::NextColumn();
        ImGui::Text("%u", manager.GetPacketsReceived());
        ImGui::NextColumn();
        
        ImGui::Text("Bytes Sent:");
        ImGui::NextColumn();
        ImGui::Text("%.2f KB", manager.GetBytesSent() / 1024.0f);
        ImGui::NextColumn();
        
        ImGui::Text("Bytes Received:");
        ImGui::NextColumn();
        ImGui::Text("%.2f KB", manager.GetBytesReceived() / 1024.0f);
        ImGui::NextColumn();
        
        ImGui::Text("Connected Peers:");
        ImGui::NextColumn();
        ImGui::Text("%zu", manager.GetPeerCount());
        ImGui::NextColumn();
        
        if (manager.IsClient()) {
            ImGui::Text("Server Latency:");
            ImGui::NextColumn();
            ImGui::Text("%u ms", manager.GetLatency(0));
            ImGui::NextColumn();
        }
        
        ImGui::Columns(1);
        
        // Performance indicators
        ImGui::Separator();
        ImGui::Text("Performance");
        
        float packetLossRate = 0.0f;
        uint32_t totalPackets = manager.GetPacketsSent() + manager.GetPacketsReceived();
        if (totalPackets > 0) {
            // Simple approximation - in a real implementation you'd track this properly
            packetLossRate = 0.0f; // ENet handles retransmission automatically
        }
        
        ImGui::Text("Packet Loss: %.2f%%", packetLossRate);
        
        // Connection quality indicator
        uint32_t latency = manager.IsClient() ? manager.GetLatency(0) : 0;
        ImVec4 qualityColor;
        std::string qualityText;
        
        if (latency == 0) {
            qualityColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            qualityText = "N/A";
        } else if (latency < 50) {
            qualityColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
            qualityText = "Excellent";
        } else if (latency < 100) {
            qualityColor = ImVec4(0.5f, 1.0f, 0.0f, 1.0f);
            qualityText = "Good";
        } else if (latency < 200) {
            qualityColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
            qualityText = "Fair";
        } else {
            qualityColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            qualityText = "Poor";
        }
        
        ImGui::Text("Connection Quality:");
        ImGui::SameLine();
        ImGui::TextColored(qualityColor, "%s", qualityText.c_str());
    }
    ImGui::EndChild();
}

void NetworkUI::DrawConnectedPeers() {
    auto& manager = Network::GetManager();
    const auto& peers = manager.GetConnectedPeers();
    
    if (peers.empty()) {
        ImGui::Text("No connected peers");
        return;
    }
    
    if (ImGui::BeginTable("PeersTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Address");
        ImGui::TableSetupColumn("Latency");
        ImGui::TableSetupColumn("Status");
        ImGui::TableHeadersRow();
        
        for (const auto& peer : peers) {
            ImGui::TableNextRow();
            
            ImGui::TableNextColumn();
            ImGui::Text("%u", peer.id);
            
            ImGui::TableNextColumn();
            ImGui::Text("%s:%u", peer.address.c_str(), peer.port);
            
            ImGui::TableNextColumn();
            if (peer.enetPeer) {
                ImGui::Text("%u ms", peer.enetPeer->roundTripTime);
            } else {
                ImGui::Text("N/A");
            }
            
            ImGui::TableNextColumn();
            if (peer.isConnected) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Connected");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Disconnected");
            }
        }
        
        ImGui::EndTable();
    }
}

void NetworkUI::DrawChatPanel() {
    auto& manager = Network::GetManager();
    
    // Only show chat if we're connected
    if (!manager.IsServer() && !manager.IsClient()) {
        ImGui::Text("Connect to a server or start a server to use chat");
        return;
    }
    
    // Chat messages display
    if (ImGui::BeginChild("ChatMessages", ImVec2(0, -60), true)) {
        for (const auto& msg : m_chatMessages) {
            if (msg.isSystemMessage) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[SYSTEM] %s", msg.message.c_str());
            } else {
                ImGui::TextWrapped("[%s]: %s", msg.playerName.c_str(), msg.message.c_str());
            }
        }
        
        // Auto-scroll to bottom
        if (m_settings.autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();
    
    // Chat input
    ImGui::SetNextItemWidth(-80);
    bool enterPressed = ImGui::InputText("##ChatInput", m_chatMessageBuffer, sizeof(m_chatMessageBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    
    if (ImGui::Button("Send") || enterPressed) {
        SendChatMessage();
    }
}

void NetworkUI::DrawNetworkLog() {
    if (ImGui::BeginChild("LogChild", ImVec2(0, -30), true)) {
        for (const auto& entry : m_logEntries) {
            ImVec4 color;
            std::string prefix;
            
            switch (entry.type) {
                case 0: // info
                    color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                    prefix = "[INFO]";
                    break;
                case 1: // warning
                    color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                    prefix = "[WARN]";
                    break;
                case 2: // error
                    color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                    prefix = "[ERROR]";
                    break;
            }
            
            // Format timestamp
            uint32_t seconds = entry.timestamp / 1000;
            uint32_t minutes = seconds / 60;
            seconds %= 60;
            
            ImGui::TextColored(color, "[%02u:%02u] %s %s", minutes, seconds, prefix.c_str(), entry.message.c_str());
        }
        
        // Auto-scroll to bottom
        if (m_settings.autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();
    
    if (ImGui::Button("Clear Log")) {
        m_logEntries.clear();
    }
}

void NetworkUI::HandleNetworkEvent(const NetworkEvent& event) {
    LogEntry logEntry;
    logEntry.timestamp = enet_time_get();
    logEntry.type = 0; // info by default
    
    ChatMessage chatMsg;
    chatMsg.timestamp = enet_time_get();
    chatMsg.isSystemMessage = true;
    
    switch (event.type) {
        case NetworkEventType::CLIENT_CONNECTED:
            logEntry.message = "Client connected: ID " + std::to_string(event.peerID);
            chatMsg.message = "Player joined the server";
            m_chatMessages.push_back(chatMsg);
            break;
            
        case NetworkEventType::CLIENT_DISCONNECTED:
            logEntry.message = "Client disconnected: ID " + std::to_string(event.peerID);
            chatMsg.message = "Player left the server";
            m_chatMessages.push_back(chatMsg);
            break;
            
        case NetworkEventType::SERVER_CONNECTED:
            logEntry.message = "Connected to server: " + event.message;
            chatMsg.message = "Connected to server";
            m_chatMessages.push_back(chatMsg);
            break;
            
        case NetworkEventType::SERVER_DISCONNECTED:
            logEntry.message = "Disconnected from server: " + event.message;
            chatMsg.message = "Disconnected from server";
            m_chatMessages.push_back(chatMsg);
            break;
            
        case NetworkEventType::CONNECTION_FAILED:
            logEntry.message = "Connection failed: " + event.message;
            logEntry.type = 2; // error
            break;
            
        case NetworkEventType::SERVER_STARTED:
            logEntry.message = "Server started: " + event.message;
            chatMsg.message = "Server started";
            m_chatMessages.push_back(chatMsg);
            break;
            
        case NetworkEventType::SERVER_STOPPED:
            logEntry.message = "Server stopped";
            chatMsg.message = "Server stopped";
            m_chatMessages.push_back(chatMsg);
            break;
    }
    
    m_logEntries.push_back(logEntry);
    
    // Limit log size
    if (m_logEntries.size() > m_settings.maxLogEntries) {
        m_logEntries.erase(m_logEntries.begin());
    }
    
    // Limit chat size
    if (m_chatMessages.size() > m_settings.maxChatMessages) {
        m_chatMessages.erase(m_chatMessages.begin());
    }
    
    // Notify external callback if set
    if (m_connectionStateCallback) {
        auto& manager = Network::GetManager();
        m_connectionStateCallback(manager.IsServer(), manager.IsClient() || manager.IsServer());
    }
}

void NetworkUI::SendChatMessage() {
    if (strlen(m_chatMessageBuffer) == 0) {
        return;
    }
    
    auto& manager = Network::GetManager();
    if (!manager.IsServer() && !manager.IsClient()) {
        return;
    }
    
    // Create chat packet
    PacketData::ChatMessage chatData;
    chatData.playerID = 1; // Simple player ID system
    chatData.playerName = std::string(m_playerNameBuffer);
    chatData.message = std::string(m_chatMessageBuffer);
    
    Packet chatPacket = PacketFactory::CreateChatPacket(chatData);
    
    if (manager.IsServer()) {
        // Server broadcasts to all clients and shows locally
        manager.BroadcastPacket(chatPacket);
        
        // Add to local chat
        ChatMessage msg;
        msg.playerName = chatData.playerName;
        msg.message = chatData.message;
        msg.timestamp = enet_time_get();
        msg.isSystemMessage = false;
        m_chatMessages.push_back(msg);
    } else if (manager.IsClient()) {
        // Client sends to server AND adds to local chat
        manager.SendPacket(chatPacket);
        
        // Add to local chat immediately so sender sees their own message
        ChatMessage msg;
        msg.playerName = chatData.playerName;
        msg.message = chatData.message;
        msg.timestamp = enet_time_get();
        msg.isSystemMessage = false;
        m_chatMessages.push_back(msg);
        
        // Limit chat history
        if (m_chatMessages.size() > m_settings.maxChatMessages) {
            m_chatMessages.erase(m_chatMessages.begin());
        }
    }
    
    // Clear input
    memset(m_chatMessageBuffer, 0, sizeof(m_chatMessageBuffer));
}

void NetworkUI::StartServer() {
    uint16_t port = static_cast<uint16_t>(atoi(m_serverPortBuffer));
    size_t maxClients = static_cast<size_t>(atoi(m_maxClientsBuffer));
    
    if (port == 0) port = 7777;
    if (maxClients == 0) maxClients = 10;
    
    if (Network::StartServer(port, maxClients)) {
        LogEntry entry;
        entry.message = "Server started on port " + std::to_string(port);
        entry.timestamp = enet_time_get();
        entry.type = 0;
        m_logEntries.push_back(entry);
    } else {
        LogEntry entry;
        entry.message = "Failed to start server: " + NetworkManager::GetLastError();
        entry.timestamp = enet_time_get();
        entry.type = 2;
        m_logEntries.push_back(entry);
    }
}

void NetworkUI::ConnectToServer() {
    std::string address(m_serverAddressBuffer);
    uint16_t port = static_cast<uint16_t>(atoi(m_clientPortBuffer));
    
    if (port == 0) port = 7777;
    if (address.empty()) address = "localhost";
    
    if (Network::ConnectToServer(address, port)) {
        LogEntry entry;
        entry.message = "Connecting to " + address + ":" + std::to_string(port);
        entry.timestamp = enet_time_get();
        entry.type = 0;
        m_logEntries.push_back(entry);
    } else {
        LogEntry entry;
        entry.message = "Failed to connect: " + NetworkManager::GetLastError();
        entry.timestamp = enet_time_get();
        entry.type = 2;
        m_logEntries.push_back(entry);
    }
}

void NetworkUI::Disconnect() {
    Logger::Info("=== NetworkUI::Disconnect() CALLED ===");
    
    auto& manager = Network::GetManager();
    
    if (manager.IsServer()) {
        Logger::Info("Stopping server...");
        manager.StopServer();
    } else if (manager.IsClient()) {
        Logger::Info("Client disconnect requested via NetworkUI");
        Logger::Info("Calling manager.DisconnectFromServer() - this bypasses Game cleanup!");
        
        manager.DisconnectFromServer("User requested disconnect");
        
        // CRITICAL FIX: Force event processing to ensure SERVER_DISCONNECTED is handled
        Logger::Info("Forcing network event processing to trigger cleanup...");
        manager.Update(); // This will process any pending events
    }
}

void NetworkUI::UpdateNetworkState() {
    auto& manager = Network::GetManager();
    
    // Update network processing
    manager.Update();
    
    // Update stats periodically
    m_stats.updateTimer += Time::DeltaTime();
    if (m_stats.updateTimer >= 1.0f) {
        m_stats.packetsSent = manager.GetPacketsSent();
        m_stats.packetsReceived = manager.GetPacketsReceived();
        m_stats.bytesSent = manager.GetBytesSent();
        m_stats.bytesReceived = manager.GetBytesReceived();
        m_stats.peerCount = manager.GetPeerCount();
        
        if (manager.IsClient()) {
            m_stats.latency = manager.GetLatency(0);
        }
        
        m_stats.updateTimer = 0.0f;
    }
} 