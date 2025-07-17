#pragma once

#include "NetworkManager.h"
#include "Packet.h"
#include <engine/utils/Logger.h>
#include <GLFW/glfw3.h>
#include <memory>

/**
 * Example class showing how to integrate networking into your game
 * This demonstrates both server and client functionality
 */
class NetworkExample {
public:
    NetworkExample() : m_IsServer(false), m_IsClient(false) {}
    
    // Initialize networking system
    bool Initialize() {
        // Initialize the global network manager
        if (!Network::Initialize()) {
            Logger::Error<NetworkExample>("Failed to initialize networking", this);
            return false;
        }
        
        // Set up event callback to handle network events
        Network::GetManager().SetEventCallback([this](const NetworkEvent& event) {
            HandleNetworkEvent(event);
        });
        
        // Register packet handlers for game-specific packets
        SetupPacketHandlers();
        
        Logger::Info("Network example initialized");
        return true;
    }
    
    // Shutdown networking
    void Shutdown() {
        Network::Shutdown();
        Logger::Info("Network example shut down");
    }
    
    // Start as server
    bool StartServer(uint16_t port = 7777) {
        if (!Network::StartServer(port, 10)) {
            Logger::Error<NetworkExample>("Failed to start server on port " + 
                                        std::to_string(port), this);
            return false;
        }
        
        m_IsServer = true;
        m_IsClient = false;
        Logger::Info("Started as server on port " + std::to_string(port));
        return true;
    }
    
    // Connect to server as client
    bool ConnectToServer(const std::string& address = "localhost", uint16_t port = 7777) {
        if (!Network::ConnectToServer(address, port)) {
            Logger::Error<NetworkExample>("Failed to connect to server " + 
                                        address + ":" + std::to_string(port), this);
            return false;
        }
        
        m_IsClient = true;
        m_IsServer = false;
        Logger::Info("Connected to server " + address + ":" + std::to_string(port));
        
        // Send a join packet
        SendPlayerJoinPacket("Player_" + std::to_string(rand() % 1000));
        return true;
    }
    
    // Update networking (call every frame)
    void Update() {
        Network::Update();
        
        // Example: Send periodic player position updates
        static uint32_t lastUpdateTime = 0;
        uint32_t currentTime = enet_time_get();
        
        if (m_IsClient && currentTime - lastUpdateTime > 50) { // 20 FPS updates
            SendPlayerPositionUpdate();
            lastUpdateTime = currentTime;
        }
    }
    
    // Handle keyboard input for networking commands
    void HandleInput() {
        // Example key bindings (you'd integrate this with your Input system)
        static bool s_KeyPressed = false;
        
        if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_F2) == GLFW_PRESS && !s_KeyPressed) {
            StartServer();
            s_KeyPressed = true;
        } else if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_F3) == GLFW_PRESS && !s_KeyPressed) {
            ConnectToServer();
            s_KeyPressed = true;
        } else if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_F4) == GLFW_PRESS && !s_KeyPressed) {
            SendChatMessage("Hello from " + (m_IsServer ? "server" : "client") + "!");
            s_KeyPressed = true;
        }
        
        if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_F2) == GLFW_RELEASE &&
            glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_F3) == GLFW_RELEASE &&
            glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_F4) == GLFW_RELEASE) {
            s_KeyPressed = false;
        }
    }
    
    // Send a chat message
    void SendChatMessage(const std::string& message) {
        PacketData::ChatMessage chatData;
        chatData.playerID = GetLocalPlayerID();
        chatData.playerName = "Player_" + std::to_string(chatData.playerID);
        chatData.message = message;
        
        Packet chatPacket = PacketFactory::CreateChatPacket(chatData);
        
        if (m_IsServer) {
            // Server broadcasts to all clients
            Network::BroadcastPacket(chatPacket);
            Logger::Info("[SERVER] " + chatData.playerName + ": " + message);
        } else if (m_IsClient) {
            // Client sends to server
            Network::SendPacket(chatPacket);
        }
    }
    
    // Send player join notification
    void SendPlayerJoinPacket(const std::string& playerName) {
        PacketData::PlayerJoin joinData;
        joinData.playerID = GetLocalPlayerID();
        joinData.playerName = playerName;
        joinData.spawnPosition = glm::vec2(400.0f, 300.0f); // Default spawn
        
        Packet joinPacket = PacketFactory::CreatePlayerJoinPacket(joinData);
        
        if (m_IsClient) {
            Network::SendPacket(joinPacket);
        }
    }
    
    // Send player position update
    void SendPlayerPositionUpdate() {
        if (!m_IsClient) return;
        
        PacketData::PlayerMove moveData;
        moveData.playerID = GetLocalPlayerID();
        moveData.position = m_PlayerPosition;
        moveData.velocity = m_PlayerVelocity;
        moveData.rotation = m_PlayerRotation;
        
        Packet movePacket = PacketFactory::CreatePlayerMovePacket(moveData);
        Network::SendPacket(movePacket, 0, PacketReliability::UNRELIABLE);
    }
    
    // Get network statistics
    void PrintNetworkStats() {
        auto& manager = Network::GetManager();
        Logger::Info("Network Stats:");
        Logger::Info("  Connected peers: " + std::to_string(manager.GetPeerCount()));
        Logger::Info("  Packets sent: " + std::to_string(manager.GetPacketsSent()));
        Logger::Info("  Packets received: " + std::to_string(manager.GetPacketsReceived()));
        Logger::Info("  Bytes sent: " + std::to_string(manager.GetBytesSent()));
        Logger::Info("  Bytes received: " + std::to_string(manager.GetBytesReceived()));
        
        if (m_IsClient) {
            Logger::Info("  Latency to server: " + std::to_string(manager.GetLatency(0)) + "ms");
        }
    }

private:
    bool m_IsServer;
    bool m_IsClient;
    
    // Example player data (you'd get this from your actual game state)
    glm::vec2 m_PlayerPosition{400.0f, 300.0f};
    glm::vec2 m_PlayerVelocity{0.0f, 0.0f};
    float m_PlayerRotation = 0.0f;
    
    // Get local player ID (simple implementation)
    uint32_t GetLocalPlayerID() const {
        return m_IsServer ? 0 : 1; // Server is always ID 0, clients get assigned IDs
    }
    
    // Set up packet handlers for game-specific packets
    void SetupPacketHandlers() {
        auto& manager = Network::GetManager();
        
        // Handle player movement packets
        manager.RegisterPacketHandler(PacketType::PLAYER_MOVE, 
            [this](const Packet& packet, uint32_t senderID) {
                HandlePlayerMovePacket(packet, senderID);
            });
        
        // Handle chat messages
        manager.RegisterPacketHandler(PacketType::CHAT_MESSAGE, 
            [this](const Packet& packet, uint32_t senderID) {
                HandleChatPacket(packet, senderID);
            });
        
        // Handle player join
        manager.RegisterPacketHandler(PacketType::PLAYER_JOIN, 
            [this](const Packet& packet, uint32_t senderID) {
                HandlePlayerJoinPacket(packet, senderID);
            });
    }
    
    // Handle network events
    void HandleNetworkEvent(const NetworkEvent& event) {
        switch (event.type) {
            case NetworkEventType::CLIENT_CONNECTED:
                Logger::Info("Client connected: ID " + std::to_string(event.peerID));
                break;
                
            case NetworkEventType::CLIENT_DISCONNECTED:
                Logger::Info("Client disconnected: ID " + std::to_string(event.peerID) + 
                           " (" + event.message + ")");
                break;
                
            case NetworkEventType::SERVER_CONNECTED:
                Logger::Info("Connected to server: " + event.message);
                break;
                
            case NetworkEventType::SERVER_DISCONNECTED:
                Logger::Info("Disconnected from server: " + event.message);
                m_IsClient = false;
                break;
                
            case NetworkEventType::CONNECTION_FAILED:
                Logger::Error<NetworkExample>("Connection failed: " + event.message, this);
                break;
                
            case NetworkEventType::SERVER_STARTED:
                Logger::Info("Server started: " + event.message);
                break;
                
            case NetworkEventType::SERVER_STOPPED:
                Logger::Info("Server stopped");
                m_IsServer = false;
                break;
                
            default:
                break;
        }
    }
    
    // Handle player movement packet
    void HandlePlayerMovePacket(const Packet& packet, uint32_t senderID) {
        PacketData::PlayerMove moveData;
        Packet mutablePacket = packet; // Make a copy to read from
        moveData.ReadFrom(mutablePacket);
        
        Logger::Info("Player " + std::to_string(moveData.playerID) + 
                    " moved to (" + std::to_string(moveData.position.x) + 
                    ", " + std::to_string(moveData.position.y) + ")");
        
        // If we're the server, broadcast this update to all other clients
        if (m_IsServer) {
            // Create a new packet and broadcast to all clients except sender
            for (const auto& peer : Network::GetManager().GetConnectedPeers()) {
                if (peer.id != senderID) {
                    Packet broadcastPacket = PacketFactory::CreatePlayerMovePacket(moveData);
                    Network::GetManager().SendPacket(broadcastPacket, peer.id, 
                                                   PacketReliability::UNRELIABLE);
                }
            }
        }
        
        // Update local representation of remote player
        // (In a real game, you'd update your player/entity system here)
    }
    
    // Handle chat message packet
    void HandleChatPacket(const Packet& packet, uint32_t senderID) {
        PacketData::ChatMessage chatData;
        Packet mutablePacket = packet; // Make a copy to read from
        chatData.ReadFrom(mutablePacket);
        
        Logger::Info("[CHAT] " + chatData.playerName + ": " + chatData.message);
        
        // If we're the server, broadcast this message to all other clients
        if (m_IsServer) {
            for (const auto& peer : Network::GetManager().GetConnectedPeers()) {
                if (peer.id != senderID) {
                    Packet broadcastPacket = PacketFactory::CreateChatPacket(chatData);
                    Network::GetManager().SendPacket(broadcastPacket, peer.id);
                }
            }
        }
    }
    
    // Handle player join packet
    void HandlePlayerJoinPacket(const Packet& packet, uint32_t senderID) {
        PacketData::PlayerJoin joinData;
        Packet mutablePacket = packet; // Make a copy to read from
        joinData.ReadFrom(mutablePacket);
        
        Logger::Info("Player " + joinData.playerName + " joined the game");
        
        // If we're the server, broadcast this to all other clients
        if (m_IsServer) {
            for (const auto& peer : Network::GetManager().GetConnectedPeers()) {
                if (peer.id != senderID) {
                    Packet broadcastPacket = PacketFactory::CreatePlayerJoinPacket(joinData);
                    Network::GetManager().SendPacket(broadcastPacket, peer.id);
                }
            }
        }
        
        // Add player to game world
        // (In a real game, you'd spawn the player entity here)
    }
};

/**
 * Usage instructions:
 * 
 * 1. Include this in your Game class or main application
 * 2. Call Initialize() after your engine initializes
 * 3. Call Update() every frame
 * 4. Use keyboard shortcuts:
 *    - F2: Start server
 *    - F3: Connect to server as client
 *    - F4: Send test chat message
 * 
 * Example integration in Game class:
 * 
 * class Game : public Engine {
 * private:
 *     std::unique_ptr<NetworkExample> m_Network;
 * 
 * public:
 *     void OnInit() override {
 *         m_Network = std::make_unique<NetworkExample>();
 *         m_Network->Initialize();
 *     }
 * 
 *     void OnUpdate() override {
 *         m_Network->Update();
 *         m_Network->HandleInput();
 *     }
 * 
 *     void OnShutdown() override {
 *         m_Network->Shutdown();
 *     }
 * };
 */ 