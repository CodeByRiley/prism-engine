#include "Game.h"

#include <glad/glad.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <GLFW/glfw3.h>
#include <engine/renderer/fog/FogRenderer2D.h>
#include <engine/renderer/vision/VisionRenderer2D.h>
#include <engine/renderer/lighting/LightRenderer2D.h>
#include <engine/renderer/lighting/Light.h>
#include <engine/renderer/QuadBatch.h>
#include <engine/renderer/Shader.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

// ImGui includes for centralized UI rendering
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (!game) return;
    Logger::Info("Framebuffer resized to: " + std::to_string(width) + "x" + std::to_string(height));
    glViewport(0, 0, width, height);
    game->OnResize(width, height);
}

void Game::setupObstacles() {
    visionRenderer->ClearObstacles();
    lightRenderer->ClearObstacles();
    fogRenderer->ClearObstacles();
    m_Obstacles.push_back(Obstacle(glm::vec2(400, 300), glm::vec2(100, 200)));
    m_Obstacles.push_back(Obstacle(glm::vec2(800, 400), glm::vec2(150, 80)));
    m_Obstacles.push_back(Obstacle(glm::vec2(200, 500), glm::vec2(120, 120)));
    m_Obstacles.push_back(Obstacle(glm::vec2(1000, 200), glm::vec2(80, 300)));
    visionRenderer->AddObstacles(m_Obstacles);
    lightRenderer->AddObstacles(m_Obstacles);
    fogRenderer->AddObstacles(m_Obstacles);
}

void Game::setupLights() {
    m_Lights.clear();
    
    // Single point light with good dispersion
    m_Lights.emplace_back(glm::vec2(640, 360), 1204.0f, glm::vec3(1.0f, 1.0f, 1.0f), 5.25f);
}

Game::Game(int width, int height, const char* title)
    : Engine(width, height, title),
      m_Player(glm::vec2(width * 0.5f, height * 0.5f), glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), 700.0f),
      windowWidth(width),
      windowHeight(height),
      renderer(nullptr),
      fogRenderer(nullptr),
      visionRenderer(nullptr),
      lightRenderer(nullptr),
      m_RenderMode(RenderMode::LIGHTING),
      m_playerMovementSystem(nullptr),
      m_localPlayerNetworkID(0)
{
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, framebufferSizeCallback);
    renderer = new Renderer2D(width, height);
    fogRenderer = new FogRenderer2D(width, height);
    visionRenderer = new VisionRenderer2D(width, height);
    lightRenderer = new LightRenderer2D(width, height);

    // Create ECS scene
    m_scene = std::make_unique<Scene>("GameScene", 1);
    
    // Setup legacy obstacles and lights (for compatibility)
    setupObstacles();
    setupLights();
    
    // Setup fog system with obstacles
    fogRenderer->AddObstacles(m_Obstacles);
    
    // Configure vision system
    m_VisionConfig.range = 1024.0f;
    m_VisionConfig.angle = 1.0472f; // 60 degrees in radians
    m_VisionConfig.shadowLength = 900.0f;
    m_VisionConfig.shadowSoftness = 0.82f;
    m_VisionConfig.darkColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.85f);
    
    // Configure lighting system
    m_LightConfig.ambientLight = 0.45f;
    m_LightConfig.ambientColor = glm::vec3(0.75f, 0.75f, 0.75f);
    m_LightConfig.shadowSoftness = 0.4f;    
    m_LightConfig.shadowLength = 1000.0f;
    m_LightConfig.enableShadows = true;
    m_LightConfig.lightType = LightType::DIRECTIONAL_LIGHT;
    m_LightConfig.bloom = 0.5f;
    
    // Setup ECS systems and entities
    SetupECSScene();
}

Game::~Game() {
    OnShutdown();
}

void Game::OnInit() {
    Logger::Info("Game Init");
    Logger::Info("Press TAB to cycle between Fog, Vision, and Lighting systems");
    Logger::Info("Use WASD to move and change facing direction");
    Logger::Info("Press F5 to save scene, F9 to load scene");
    Logger::Info("Press F1 to toggle ECS Inspector");
    Logger::Info("Press F6 to toggle Network UI");
    Logger::Info("Press F7 to disconnect from server");
    Logger::Info("Press M to toggle background music");
    Logger::Info("Press N to play UI click sound");
    Logger::Info("Press B to play item pickup sound");
    Logger::Info("Press +/- to adjust master volume");
    
    // Initialize Inspector UI
    m_inspectorUI = std::make_unique<GameInspectorUI>();
    if (!m_inspectorUI->Initialize(m_Window)) {
        Logger::Error<Game>("Failed to initialize Inspector UI", this);
    } else {
        // Set up entity destruction callback
        m_inspectorUI->SetEntityDestructionCallback([this](EntityID entityID) {
            m_scene->DestroyEntity(entityID);
        });
    }
    
    // Initialize Network UI
    m_networkUI = std::make_unique<NetworkUI>();
    if (!m_networkUI->Initialize(m_Window)) {
        Logger::Error<Game>("Failed to initialize Network UI", this);
    }
    
    // Initialize Audio System
    SetupAudioSystem();
    
    // Set up networking packet handlers for game events
    SetupNetworkingHandlers();
}

void Game::SetupNetworkingHandlers() {
    auto& manager = Network::GetManager();
    
    // Handle player join events
    manager.RegisterPacketHandler(PacketType::PLAYER_JOIN,
        [this](const Packet& packet, uint32_t senderID) {
            PacketData::PlayerJoin joinData;
            Packet mutablePacket = packet;
            joinData.ReadFrom(mutablePacket);
            
            // Determine the actual player ID:
            // - If sender is server (0), use the packet data's playerID (server forwarding existing player info)
            // - If sender is client, use senderID (direct join from that client)
            uint32_t actualPlayerID;
            if (senderID == 0) {
                // Server is forwarding existing player info, use packet data
                actualPlayerID = joinData.playerID;
            } else {
                // Direct join from client, use sender ID
                actualPlayerID = senderID;
            }
            
            Logger::Info("Processing PLAYER_JOIN packet for player " + joinData.playerName + 
                        " (senderID: " + std::to_string(senderID) + ", packetID: " + std::to_string(joinData.playerID) + ", actualID: " + std::to_string(actualPlayerID) + ")");
            
            // Don't create a network entity for ourselves - we already have a local player entity
            auto& networkManager = Network::GetManager();
            if (actualPlayerID == networkManager.GetLocalPeerID()) {
                Logger::Info("Skipping network entity creation for self (player ID: " + std::to_string(actualPlayerID) + ")");
                return;
            }
            
            // Create new player entity for the joining player
            Entity newPlayer = m_scene->CreateEntity("NetworkPlayer_" + std::to_string(actualPlayerID));
            
            // Add Transform component
            newPlayer.AddComponent<TransformComponent>(
                glm::vec3(joinData.spawnPosition.x, joinData.spawnPosition.y, 0.0f)
            );
            
            // Add Renderable component FIRST and configure it immediately
            auto* renderable = newPlayer.AddComponent<RenderableComponent>();
            if (renderable) {
                // Use different colors for different clients to distinguish them
                if (actualPlayerID == 0) {
                    renderable->color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f); // Purple for server
                } else {
                    // Generate a color based on player ID
                    float hue = (actualPlayerID * 137.508f); // Golden angle for good color distribution
                    while (hue > 360.0f) hue -= 360.0f;
                    float r = std::abs(std::sin(hue * 0.017453f)) * 0.8f + 0.2f;
                    float g = std::abs(std::sin((hue + 120.0f) * 0.017453f)) * 0.8f + 0.2f;
                    float b = std::abs(std::sin((hue + 240.0f) * 0.017453f)) * 0.8f + 0.2f;
                    renderable->color = glm::vec4(r, g, b, 1.0f);
                }
                renderable->visible = true; // Ensure it's visible
                Logger::Info("RenderableComponent added and configured for network player");
            } else {
                Logger::Error<Game>("Failed to add RenderableComponent to network player", this);
            }
            
            // Add Player component
            auto* playerComp = newPlayer.AddComponent<PlayerComponent>();
            if (playerComp) {
                playerComp->speed = 700.0f;
                playerComp->size = glm::vec2(32.0f, 32.0f);
                Logger::Info("PlayerComponent added and configured for network player");
            } else {
                Logger::Error<Game>("Failed to add PlayerComponent to network player", this);
            }
            
            // Add Tag component with joining player id
            newPlayer.AddComponent<TagComponent>("network_player_" + std::to_string(actualPlayerID));
            
            // Store the mapping of network ID to entity
            m_networkPlayers[actualPlayerID] = newPlayer;
            
            Logger::Info("Network player " + joinData.playerName + " joined and entity created (ID: " + std::to_string(actualPlayerID) + ", EntityID: " + std::to_string(newPlayer.GetID()) + ")");
            Logger::Info("Total network players now: " + std::to_string(m_networkPlayers.size()));
            
            // If we're the server, broadcast this new player's join to all OTHER clients
            if (Network::GetManager().IsServer() && actualPlayerID != 0) {
                Logger::Info("Server broadcasting new client " + std::to_string(actualPlayerID) + " to all other clients");
                
                // Create a new packet with the actual player data
                PacketData::PlayerJoin broadcastData;
                broadcastData.playerID = actualPlayerID;
                broadcastData.playerName = "Player_" + std::to_string(actualPlayerID);
                broadcastData.spawnPosition = joinData.spawnPosition;
                
                Packet broadcastPacket = PacketFactory::CreatePlayerJoinPacket(broadcastData);
                
                // Send to all clients EXCEPT the one who just joined
                auto& manager = Network::GetManager();
                for (const auto& peerInfo : manager.GetConnectedPeers()) {
                    if (peerInfo.id != actualPlayerID) {
                        Network::GetManager().SendPacket(broadcastPacket, peerInfo.id);
                        Logger::Info("Sent new player " + std::to_string(actualPlayerID) + " info to client " + std::to_string(peerInfo.id));
                    }
                }
            }
            
            // Verify the entity has all required components
            bool hasTransform = newPlayer.GetComponent<TransformComponent>() != nullptr;
            bool hasPlayer = newPlayer.GetComponent<PlayerComponent>() != nullptr;
            bool hasRenderable = newPlayer.GetComponent<RenderableComponent>() != nullptr;
            
            Logger::Info("Entity components check - Transform: " + std::string(hasTransform ? "YES" : "NO") +
                        ", Player: " + std::string(hasPlayer ? "YES" : "NO") +
                        ", Renderable: " + std::string(hasRenderable ? "YES" : "NO"));
        });
    
    // Handle player leave events
    manager.RegisterPacketHandler(PacketType::PLAYER_LEAVE,
        [this](const Packet& packet, uint32_t senderID) {
            Packet mutablePacket = packet;
            uint32_t playerID = mutablePacket.ReadUint32();
            
            auto it = m_networkPlayers.find(playerID);
            if (it != m_networkPlayers.end()) {
                // Immediately make the entity invisible to prevent ghost rendering
                auto* renderable = it->second.GetComponent<RenderableComponent>();
                if (renderable) {
                    renderable->visible = false;
                }
                
                m_scene->DestroyEntity(it->second.GetID());
                m_networkPlayers.erase(it);
                Logger::Info("Network player disconnected (ID: " + std::to_string(playerID) + ")");
            }
        });
    
    // Handle player movement updates
    manager.RegisterPacketHandler(PacketType::PLAYER_MOVE,
        [this](const Packet& packet, uint32_t senderID) {
            PacketData::PlayerMove moveData;
            Packet mutablePacket = packet;
            moveData.ReadFrom(mutablePacket);
            
            // Use senderID instead of packet playerID for consistency
            uint32_t actualPlayerID = senderID;
            
            // Debug logging
            static int moveLogCounter = 0;
            if (moveLogCounter++ % 60 == 0) { // Log every 60 frames
                Logger::Info("PLAYER_MOVE received: senderID=" + std::to_string(senderID) + 
                            ", packetPlayerID=" + std::to_string(moveData.playerID) + 
                            ", pos=(" + std::to_string(moveData.position.x) + "," + std::to_string(moveData.position.y) + ")" +
                            ", networkPlayers.size=" + std::to_string(m_networkPlayers.size()));
            }
            
            // If we're the server and this is from a client, broadcast to all clients using proper broadcast
            if (Network::GetManager().IsServer() && senderID != 0) {
                // Create a new movement packet with correct player ID and send to other clients
                PacketData::PlayerMove relayMoveData;
                relayMoveData.playerID = actualPlayerID; // Use the actual sender's peer ID
                relayMoveData.position = moveData.position;
                relayMoveData.velocity = moveData.velocity;
                relayMoveData.rotation = moveData.rotation;
                
                Packet relayPacket = PacketFactory::CreatePlayerMovePacket(relayMoveData);
                
                // Send to all clients EXCEPT the original sender
                auto& relayManager = Network::GetManager();
                for (const auto& peerInfo : relayManager.GetConnectedPeers()) {
                    if (peerInfo.id != senderID) { // Don't send back to sender
                        relayManager.SendPacket(relayPacket, peerInfo.id);
                    }
                }
                
                if (moveLogCounter % 60 == 0) {
                    Logger::Info("Server broadcasting movement from client " + std::to_string(senderID) + " to other clients");
                }
            }
            
            auto it = m_networkPlayers.find(actualPlayerID);
            if (it != m_networkPlayers.end()) {
                auto* transform = it->second.GetComponent<TransformComponent>();
                auto* playerComp = it->second.GetComponent<PlayerComponent>();
                
                if (transform && playerComp) {
                    // Update position smoothly
                    transform->position = glm::vec3(moveData.position.x, moveData.position.y, 0.0f);
                    transform->rotation.z = moveData.rotation;
                    
                    // Update player direction
                    float dirX = cos(moveData.rotation);
                    float dirY = sin(moveData.rotation);
                    playerComp->direction = glm::vec2(dirX, dirY);
                    
                    if (moveLogCounter % 60 == 0) {
                        Logger::Info("Updated player " + std::to_string(actualPlayerID) + " position to (" + 
                                    std::to_string(transform->position.x) + "," + std::to_string(transform->position.y) + ")");
                    }
                }
            } else {
                Logger::Warn<Game>("Received movement for unknown player ID: " + std::to_string(actualPlayerID) + 
                                   " (available players: ", this);
                for (const auto& pair : m_networkPlayers) {
                    Logger::Info("  - Player ID: " + std::to_string(pair.first));
                }
            }
        });
    
    // Set up network event handler for connection management
    manager.SetEventCallback([this](const NetworkEvent& event) {
        switch (event.type) {
            case NetworkEventType::CLIENT_CONNECTED:
                Logger::Info("=== CLIENT CONNECTED ===");
                Logger::Info("Client connected from " + event.message + ", Peer ID: " + std::to_string(event.peerID));
                Logger::Info("Total clients now: " + std::to_string(Network::GetManager().GetPeerCount()));
                
                // If we're the server, handle new client connection
                if (Network::GetManager().IsServer()) {
                    Logger::Info("Server handling new client connection...");
                    
                    // Send ALL existing players to the new client
                    SendAllPlayersToClient(event.peerID);
                    
                    // Now wait for the new client to send their PLAYER_JOIN packet
                    // which will be handled by the PLAYER_JOIN handler and broadcasted to all clients
                }
                break;
                
            case NetworkEventType::CLIENT_DISCONNECTED:
                Logger::Info("=== CLIENT DISCONNECTED ===");
                Logger::Info("Client disconnected: " + event.message + ", Peer ID: " + std::to_string(event.peerID));
                Logger::Info("Total clients now: " + std::to_string(Network::GetManager().GetPeerCount()));
                
                // If we're the server, handle the disconnection
                if (Network::GetManager().IsServer()) {
                    Logger::Info("Server handling client disconnection...");
                    // Remove the player from the network players map
                    auto it = m_networkPlayers.find(event.peerID);
                    if (it != m_networkPlayers.end()) {
                        Logger::Info("Found player entity to remove (ID: " + std::to_string(event.peerID) + ", EntityID: " + std::to_string(it->second.GetID()) + ")");
                        m_scene->DestroyEntity(it->second.GetID());
                        m_networkPlayers.erase(it);
                        Logger::Info("Removed disconnected player entity");
                    } else {
                        Logger::Info("No player entity found for disconnected peer ID: " + std::to_string(event.peerID));
                    }
                    
                    // Notify other clients about the disconnection
                    Logger::Info("Notifying other clients about disconnection...");
                    SendPlayerLeaveToClients(event.peerID);
                }
                break;
                
            case NetworkEventType::SERVER_STARTED:
                Logger::Info("Server started on " + event.message);
                // Server always has ID 0
                m_localPlayerNetworkID = 0;
                break;
                
            case NetworkEventType::SERVER_CONNECTED:
                Logger::Info("=== CONNECTED TO SERVER ===");
                Logger::Info("Connected to server: " + event.message);
                // Client gets their peer ID from the event
                m_localPlayerNetworkID = event.peerID;
                Logger::Info("Assigned client network ID: " + std::to_string(m_localPlayerNetworkID));
                Logger::Info("Current network players at connection: " + std::to_string(m_networkPlayers.size()));
                
                // The server will first send us info about all existing players via SendAllPlayersToClient
                // Then we send our player join packet to server
                Logger::Info("Sending player join packet to server...");
                SendPlayerJoinToServer();
                break;
                
            case NetworkEventType::SERVER_DISCONNECTED:
                Logger::Info("=== SERVER DISCONNECTED ===");
                Logger::Info("Disconnected from server: " + event.message);
                Logger::Info("Network players before SERVER_DISCONNECTED cleanup: " + std::to_string(m_networkPlayers.size()));
                
                // Reset our network ID
                m_localPlayerNetworkID = 0;
                Logger::Info("Reset network ID to 0");
                
                // Clear all network players (this should remove server player if still present)
                Logger::Info("Clearing all network players...");
                ClearNetworkPlayers();
                
                break;
        }
    });
    
    Logger::Info("Network packet handlers initialized");
}

void Game::SendPlayerJoinToServer() {
    // Check if we're the client
    if (!Network::GetManager().IsClient()) {
        return;
    }
    
    // Get our local player position
    glm::vec2 spawnPos(windowWidth * 0.5f, windowHeight * 0.5f);
    if (m_playerEntity.IsValid()) {
        auto* transform = m_playerEntity.GetComponent<TransformComponent>();
        if (transform) {
            spawnPos = glm::vec2(transform->position);
        }
    }
    
    // Create player join packet
    PacketData::PlayerJoin joinData;
    // Use NetworkManager's assigned peer ID
    joinData.playerID = Network::GetManager().GetLocalPeerID();
    joinData.playerName = "Player_" + std::to_string(joinData.playerID);
    joinData.spawnPosition = spawnPos;
    
    Packet joinPacket = PacketFactory::CreatePlayerJoinPacket(joinData);
    Network::GetManager().SendPacket(joinPacket);
    
    Logger::Info("Sent player join packet to server");
}

void Game::SendPlayerLeaveToServer(uint32_t playerID) {
    // Check if we're the client
    if (!Network::GetManager().IsClient()) {
        return;
    }
    // Send leave packet to server
    Packet leavePacket = PacketFactory::CreatePlayerLeavePacket(playerID);
    Network::GetManager().SendPacket(leavePacket);
    // Clear all network players
    ClearNetworkPlayers();
    Logger::Info("Sent player leave packet to server for player ID: " + std::to_string(playerID));
}

void Game::SendPlayerLeaveToClients(uint32_t playerID) {
    // Check if we're the server
    if (!Network::GetManager().IsServer()) {
        return;
    }
    
    Packet leavePacket = PacketFactory::CreatePlayerLeavePacket(playerID);
    Network::GetManager().BroadcastPacket(leavePacket);
    Logger::Info("Broadcasted player leave packet to clients for player ID: " + std::to_string(playerID));
}

void Game::SendPlayerJoinToClients() {
    if (!Network::GetManager().IsServer()) {
        return;
    }
    
    // Get server player position (local player)
    glm::vec2 serverPos(windowWidth * 0.5f, windowHeight * 0.5f);
    if (m_playerEntity.IsValid()) {
        auto* transform = m_playerEntity.GetComponent<TransformComponent>();
        if (transform) {
            serverPos = glm::vec2(transform->position);
        }
    }
    
    // Create player join packet for server player
    PacketData::PlayerJoin joinData;
    joinData.playerID = 0; // Server host player is always ID 0
    joinData.playerName = "Player_"+std::to_string(joinData.playerID);
    joinData.spawnPosition = serverPos; // Set host position
    
    Packet joinPacket = PacketFactory::CreatePlayerJoinPacket(joinData);
    Network::GetManager().BroadcastPacket(joinPacket);
    
    Logger::Info("Broadcasted server player join packet to clients");
}

void Game::SendAllPlayersToClient(uint32_t clientID) {
    if (!Network::GetManager().IsServer()) {
        return;
    }
    
    Logger::Info("=== SENDING ALL PLAYERS TO NEW CLIENT " + std::to_string(clientID) + " ===");
    
    // Send server player info to the new client
    glm::vec2 serverPos(windowWidth * 0.5f, windowHeight * 0.5f);
    if (m_playerEntity.IsValid()) {
        auto* transform = m_playerEntity.GetComponent<TransformComponent>();
        if (transform) {
            serverPos = glm::vec2(transform->position);
        }
    }
    
    // Send server player join packet
    PacketData::PlayerJoin serverJoinData;
    serverJoinData.playerID = 0; // Server is always ID 0
    serverJoinData.playerName = "Player_0";
    serverJoinData.spawnPosition = serverPos;
    
    Packet serverJoinPacket = PacketFactory::CreatePlayerJoinPacket(serverJoinData);
    Network::GetManager().SendPacket(serverJoinPacket, clientID);
    Logger::Info("Sent server player info to client " + std::to_string(clientID));
    
    // Send info about all other connected clients to the new client
    auto& manager = Network::GetManager();
    for (const auto& peerInfo : manager.GetConnectedPeers()) {
        if (peerInfo.id != clientID && peerInfo.id != 0) { // Don't send to self or server
            // Find the existing player entity for this peer
            auto it = m_networkPlayers.find(peerInfo.id);
            if (it != m_networkPlayers.end() && it->second.IsValid()) {
                auto* transform = it->second.GetComponent<TransformComponent>();
                if (transform) {
                    PacketData::PlayerJoin existingPlayerData;
                    existingPlayerData.playerID = peerInfo.id;
                    existingPlayerData.playerName = "Player_" + std::to_string(peerInfo.id);
                    existingPlayerData.spawnPosition = glm::vec2(transform->position);
                    
                    Packet existingPlayerPacket = PacketFactory::CreatePlayerJoinPacket(existingPlayerData);
                    Network::GetManager().SendPacket(existingPlayerPacket, clientID);
                    Logger::Info("Sent existing player " + std::to_string(peerInfo.id) + " info to new client " + std::to_string(clientID));
                }
            }
        }
    }
    
    Logger::Info("=== FINISHED SENDING ALL PLAYERS TO CLIENT " + std::to_string(clientID) + " ===");
}

void Game::SendPlayerMovement() {
    auto& manager = Network::GetManager();
    if (!manager.IsServer() && !manager.IsClient()) {
        return;
    }
    
    if (!m_playerEntity.IsValid()) {
        return;
    }
    
    auto* transform = m_playerEntity.GetComponent<TransformComponent>();
    auto* playerComp = m_playerEntity.GetComponent<PlayerComponent>();
    
    if (!transform || !playerComp) {
        return;
    }
    
    // Create movement packet
    PacketData::PlayerMove moveData;
    moveData.playerID = manager.GetLocalPeerID(); // Use NetworkManager's assigned peer ID
    moveData.position = glm::vec2(transform->position);
    moveData.velocity = glm::vec2(0.0f); // Could calculate velocity
    moveData.rotation = transform->rotation.z;
    
    // Debug: Check if we have a valid peer ID before sending (server can have ID 0)
    if (moveData.playerID == 0 && !manager.IsServer()) {
        static int warningCounter = 0;
        if (warningCounter++ % 120 == 0) { // Log every 2 seconds
            Logger::Warn<Game>("CLIENT SendPlayerMovement called with peer ID 0! " +
                              std::string("NetworkManager localPeerID: ") + std::to_string(manager.GetLocalPeerID()), this);
        }
        return; // Don't send movement with invalid peer ID for clients
    }
    
    Packet movePacket = PacketFactory::CreatePlayerMovePacket(moveData);
    
    // Debug logging
    static int logCounter = 0;
    if (logCounter++ % 60 == 0) { // Log every 60 frames (about 1 second)
        Logger::Info("SendPlayerMovement: playerID=" + std::to_string(moveData.playerID) + 
                    ", pos=(" + std::to_string(moveData.position.x) + "," + std::to_string(moveData.position.y) + ")" +
                    ", mode=" + std::string(manager.IsServer() ? "SERVER" : "CLIENT"));
    }
    
    if (manager.IsServer()) {
        manager.BroadcastPacket(movePacket);
    } else {
        manager.SendPacket(movePacket);
    }
}

void Game::ClearNetworkPlayers() {
    Logger::Info("Clearing " + std::to_string(m_networkPlayers.size()) + " network players:");
    for (auto& pair : m_networkPlayers) {
        Logger::Info("  - Removing player ID: " + std::to_string(pair.first) + ", Entity ID: " + std::to_string(pair.second.GetID()));
        if (pair.second.IsValid()) {
            // Immediately make the entity invisible to prevent ghost rendering
            auto* renderable = pair.second.GetComponent<RenderableComponent>();
            if (renderable) {
                renderable->visible = false;
                Logger::Info("-   Made entity invisible before destruction");
            }
            
            m_scene->DestroyEntity(pair.second.GetID());
        }
    }
    m_networkPlayers.clear();
    Logger::Info("All network players cleared");
}

void Game::DisconnectFromServer() {
    Logger::Info("=== DisconnectFromServer() CALLED ===");
    
    auto& manager = Network::GetManager();
    
    Logger::Info("Checking if we're a client...");
    Logger::Info("manager.IsClient() = " + std::string(manager.IsClient() ? "TRUE" : "FALSE"));
    Logger::Info("manager.IsServer() = " + std::string(manager.IsServer() ? "TRUE" : "FALSE"));
    
    if (!manager.IsClient()) {
        Logger::Info("Not connected to server, nothing to disconnect from");
        return;
    }
    
    Logger::Info("=== INITIATING CLIENT DISCONNECT ===");
    Logger::Info("Current network players before disconnect: " + std::to_string(m_networkPlayers.size()));
    
    // Clear network players immediately as safety measure
    // The SERVER_DISCONNECTED event may not be processed due to thread shutdown timing
    Logger::Info("Clearing network players before disconnect...");
    ClearNetworkPlayers();
    
    // Reset our network ID
    m_localPlayerNetworkID = 0;
    Logger::Info("Reset local player network ID to 0");
    
    // Disconnect from server (this is already threaded in NetworkManager)
    manager.DisconnectFromServer("Client disconnecting");
    
    Logger::Info("Disconnect command sent, network players cleared");
}

void Game::SetupECSScene() {
    // Register all systems
    m_playerMovementSystem = m_scene->RegisterSystem<PlayerMovementSystem>(windowWidth, windowHeight);
    
    // Create player entity
    m_playerEntity = m_scene->CreateEntity("Player");
    m_playerEntity.AddComponent<TransformComponent>(
        glm::vec3(windowWidth * 0.5f, windowHeight * 0.5f, 0.0f)
    );
    m_playerEntity.AddComponent<RenderableComponent>();
    auto* playerComp = m_playerEntity.AddComponent<PlayerComponent>();
    if (playerComp) {
        playerComp->speed = 700.0f;
        playerComp->size = glm::vec2(32.0f, 32.0f);
    }
    m_playerEntity.AddComponent<InputComponent>();
    m_playerEntity.AddComponent<TagComponent>("player");
    
    // Set player color (purple like original)
    auto* playerRenderable = m_playerEntity.GetComponent<RenderableComponent>();
    if (playerRenderable) {
        playerRenderable->color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
    }
    
    // Setup obstacles and lights
    SetupECSObstacles();
    SetupECSLights();
    
    Logger::Info("ECS Scene setup complete with " + 
                std::to_string(m_scene->GetAllEntities().size()) + " entities");
}

void Game::SetupECSObstacles() {
    // Clear old obstacle data from renderers
    visionRenderer->ClearObstacles();
    lightRenderer->ClearObstacles();
    fogRenderer->ClearObstacles();
    
    // Create obstacle entities matching the original positions
    struct ObstacleData {
        glm::vec2 position;
        glm::vec2 size;
    };
    
    std::vector<ObstacleData> obstacleData = {
        {glm::vec2(400, 300), glm::vec2(100, 200)},
        {glm::vec2(800, 400), glm::vec2(150, 80)},
        {glm::vec2(200, 500), glm::vec2(120, 120)},
        {glm::vec2(1000, 200), glm::vec2(80, 300)}
    };
    
    for (size_t i = 0; i < obstacleData.size(); ++i) {
        Entity obstacle = m_scene->CreateEntity("Obstacle_" + std::to_string(i));
        obstacle.AddComponent<TransformComponent>(
            glm::vec3(obstacleData[i].position.x, obstacleData[i].position.y, 0.0f)
        );
        obstacle.AddComponent<ObstacleComponent>(obstacleData[i].size);
        auto* renderable = obstacle.AddComponent<RenderableComponent>();
        if (renderable) {
            renderable->color = glm::vec4(1.0f, 0.25f, 0.45f, 1.0f);
        }
        obstacle.AddComponent<TagComponent>("obstacle");
    }
    
    // Update renderers with new obstacle data
    UpdateRenderersFromECS();
}

void Game::SetupECSLights() {
    // Create light entity matching the original setup
    Entity light = m_scene->CreateEntity("MainLight");
    light.AddComponent<TransformComponent>(glm::vec3(640, 360, 0.0f));
    
    // Create a proper Light struct for the LightComponent
    Light lightData(glm::vec2(640, 360), 1204.0f, glm::vec3(1.0f, 1.0f, 1.0f), 5.25f);
    light.AddComponent<LightComponent>(lightData);
    
    light.AddComponent<TagComponent>("light");
}

void Game::UpdateRenderersFromECS() {
    // Update renderer obstacle lists from ECS entities
    std::vector<Obstacle> obstacles;
    
    auto obstacleEntities = m_scene->GetEntitiesWith<TransformComponent, ObstacleComponent>();
    for (const Entity& entity : obstacleEntities) {
        auto* transform = entity.GetComponent<TransformComponent>();
        auto* obstacleComp = entity.GetComponent<ObstacleComponent>();
        
        if (transform && obstacleComp) {
            obstacles.emplace_back(
                glm::vec2(transform->position), 
                obstacleComp->size
            );
        }
    }
    
    // Update all renderers
    visionRenderer->AddObstacles(obstacles);
    lightRenderer->AddObstacles(obstacles);
    fogRenderer->AddObstacles(obstacles);
    
    // Update lights
    m_Lights.clear();
    auto lightEntities = m_scene->GetEntitiesWith<TransformComponent, LightComponent>();
    for (const Entity& entity : lightEntities) {
        auto* transform = entity.GetComponent<TransformComponent>();
        auto* lightComp = entity.GetComponent<LightComponent>();
        
        if (transform && lightComp) {
            m_Lights.emplace_back(
                glm::vec2(transform->position),
                lightComp->light.range,
                lightComp->light.color,
                lightComp->light.intensity
            );
        }
    }
}

void Game::OnUpdate() {
    float deltaTime = Time::DeltaTime();
    
    if (Input::IsKeyPressed(GLFW_KEY_ESCAPE)) {
        // Why? this should call shutdown
        //this->m_Running = false;
        this->OnShutdown();
    }
    
    // Cycle between rendering systems
    if (Input::IsKeyPressed(GLFW_KEY_TAB)) {
        m_RenderMode = static_cast<RenderMode>((static_cast<int>(m_RenderMode) + 1) % 3);
        switch(m_RenderMode) {
            case RenderMode::FOG: Logger::Info("Fog system enabled"); break;
            case RenderMode::VISION: Logger::Info("Vision system enabled"); break;
            case RenderMode::LIGHTING: Logger::Info("Lighting system enabled"); break;
        }
    }
    
    // Toggle Inspector
    if (Input::IsKeyPressed(GLFW_KEY_F1)) {
        if (m_inspectorUI) {
            m_inspectorUI->ToggleVisibility();
            Logger::Info("Inspector " + std::string(m_inspectorUI->IsVisible() ? "ENABLED" : "DISABLED"));
        }
    }
    
    // Toggle Network UI
    if (Input::IsKeyPressed(GLFW_KEY_F6)) {
        if (m_networkUI) {
            m_networkUI->ToggleVisibility();
            Logger::Info("Network UI " + std::string(m_networkUI->IsVisible() ? "ENABLED" : "DISABLED"));
        }
    }
    
    // Disconnect from server
    if (Input::IsKeyPressed(GLFW_KEY_F7)) {
        Logger::Info("=== F7 KEY PRESSED ===");
        DisconnectFromServer();
    }
    
    // Audio controls
    static bool mKeyPressed = false;
    static bool nKeyPressed = false;
    static bool bKeyPressed = false;
    static bool plusKeyPressed = false;
    static bool minusKeyPressed = false;
    
    // Toggle music playback with M key
    if (Input::IsKeyHeld(GLFW_KEY_M) && !mKeyPressed) {
        mKeyPressed = true;
        if (Audio::GetManager().IsMusicPlaying("game_music")) {
            Audio::StopMusic("game_music");
            Logger::Info("Music stopped");
        } else {
            Audio::PlayMusic("game_music", true);
            Logger::Info("Music started");
        }
    } else if (!Input::IsKeyHeld(GLFW_KEY_M)) {
        mKeyPressed = false;
    }
    
    // Play sound effect 1 with N key
    if (Input::IsKeyHeld(GLFW_KEY_N) && !nKeyPressed) {
        nKeyPressed = true;
        Audio::PlaySound("gui_click");
        Logger::Info("Played gui_click sound");
    } else if (!Input::IsKeyHeld(GLFW_KEY_N)) {
        nKeyPressed = false;
    }
    
    // Play sound effect 2 with B key
    if (Input::IsKeyHeld(GLFW_KEY_B) && !bKeyPressed) {
        bKeyPressed = true;
        Audio::PlaySound("gui_check");
        Logger::Info("Played gui_check sound");
    } else if (!Input::IsKeyHeld(GLFW_KEY_B)) {
        bKeyPressed = false;
    }
    
    // Volume controls
    if (Input::IsKeyHeld(GLFW_KEY_EQUAL) && !plusKeyPressed) {
        plusKeyPressed = true;
        float volume = Audio::GetManager().GetMasterVolume();
        volume = std::min(volume + 0.1f, 1.0f);
        Audio::SetMasterVolume(volume);
        Logger::Info("Master volume: " + std::to_string(volume));
    } else if (!Input::IsKeyHeld(GLFW_KEY_EQUAL)) {
        plusKeyPressed = false;
    }
    
    if (Input::IsKeyHeld(GLFW_KEY_MINUS) && !minusKeyPressed) {
        minusKeyPressed = true;
        float volume = Audio::GetManager().GetMasterVolume();
        volume = std::max(volume - 0.1f, 0.0f);
        Audio::SetMasterVolume(volume);
        Logger::Info("Master volume: " + std::to_string(volume));
    } else if (!Input::IsKeyHeld(GLFW_KEY_MINUS)) {
        minusKeyPressed = false;
    }
    
    // Save/Load scene
    if (Input::IsKeyPressed(GLFW_KEY_F5)) {
        bool success = m_scene->SaveToFile("game_scene.yaml");
        Logger::Info("Scene save: " + std::string(success ? "SUCCESS" : "FAILED"));
    }
    
    if (Input::IsKeyPressed(GLFW_KEY_F9)) {
        bool success = m_scene->LoadFromFile("game_scene.yaml");
        if (success) {
            Logger::Info("Scene loaded successfully");
            // Re-setup systems and update renderers
            m_playerMovementSystem = m_scene->GetSystem<PlayerMovementSystem>();
            if (!m_playerMovementSystem) {
                m_playerMovementSystem = m_scene->RegisterSystem<PlayerMovementSystem>(windowWidth, windowHeight);
            }
            
            // Find the player entity again
            auto playerEntities = m_scene->GetEntitiesWith<PlayerComponent>();
            if (!playerEntities.empty()) {
                m_playerEntity = playerEntities[0];
            }
            
            UpdateRenderersFromECS();
        } else {
            Logger::Error<Game>("Failed to load scene", this);
        }
    }
    
    // Update ECS scene
    m_scene->Update(deltaTime);
    
    // Update Audio System
    Audio::Update();
    
    // Send movement updates if connected to network
    static float movementUpdateTimer = 0.0f;
    movementUpdateTimer += deltaTime;
    if (movementUpdateTimer >= 0.0078125f) { // Send movement updates 128 times per second
        SendPlayerMovement();
        movementUpdateTimer = 0.0f;
    }
    
    // Update network UI if it exists
    if (m_networkUI && m_networkUI->IsVisible()) {
        // Don't call Render() here, we'll handle all ImGui rendering in OnDraw
    }
    
    // Update inspector UI if it exists
    if (m_inspectorUI && m_inspectorUI->IsVisible()) {
        // Don't call Render() here, we'll handle all ImGui rendering in OnDraw
    }
}

void Game::OnDraw() {
    if (m_isShuttingDown) {
        Logger::Info("Game is shutting down, skipping draw");
        return;
    }

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Begin batch rendering
    renderer->BeginBatch(renderer->GetBaseShader());
    
    // Draw player from ECS
    auto playerEntities = m_scene->GetEntitiesWith<TransformComponent, PlayerComponent, RenderableComponent>();
    
    // Debug: Log how many player entities we found
    static int lastPlayerCount = -1;
    int currentPlayerCount = static_cast<int>(playerEntities.size());
    if (currentPlayerCount != lastPlayerCount) {
        Logger::Info("Found " + std::to_string(currentPlayerCount) + " player entities to render");
        lastPlayerCount = currentPlayerCount;
    }
    
    for (const Entity& entity : playerEntities) {
        auto* transform = entity.GetComponent<TransformComponent>();
        auto* player = entity.GetComponent<PlayerComponent>();
        auto* renderable = entity.GetComponent<RenderableComponent>();
        
        if (transform && player && renderable && renderable->visible) {
            glm::vec2 position(transform->position);
            renderer->DrawRectRot(position, player->size, transform->rotation.z, renderable->color);
            
            // Draw direction indicator
            glm::vec2 directionPos = player->GetDirectionIndicatorPos(position);
            glm::vec4 indicatorColor(-renderable->color.x, -renderable->color.y, -renderable->color.z, 1.0f);
            renderer->DrawRect(directionPos, glm::vec2(8, 8), indicatorColor);
        } else {
            // Debug: Log why entity wasn't rendered
            if (!transform) Logger::Info("Entity " + std::to_string(entity.GetID()) + " missing TransformComponent");
            if (!player) Logger::Info("Entity " + std::to_string(entity.GetID()) + " missing PlayerComponent");
            if (!renderable) Logger::Info("Entity " + std::to_string(entity.GetID()) + " missing RenderableComponent");
            if (renderable && !renderable->visible) Logger::Info("Entity " + std::to_string(entity.GetID()) + " not visible");
        }
    }
    
    // Draw obstacles from ECS
    auto obstacleEntities = m_scene->GetEntitiesWith<TransformComponent, ObstacleComponent, RenderableComponent>();
    for (const Entity& entity : obstacleEntities) {
        auto* transform = entity.GetComponent<TransformComponent>();
        auto* obstacle = entity.GetComponent<ObstacleComponent>();
        auto* renderable = entity.GetComponent<RenderableComponent>();
        
        if (transform && obstacle && renderable && renderable->visible) {
            renderer->DrawRect(glm::vec2(transform->position), obstacle->size, renderable->color);
        }
    }
    
    renderer->EndBatch();

    // Enable blending for overlays
    glEnable(GL_BLEND);
    
    // Get player position for rendering systems
    glm::vec2 playerPos(0.0f);
    glm::vec2 playerDirection(0.0f, -1.0f);
    
    if (m_playerEntity.IsValid()) {
        auto* transform = m_playerEntity.GetComponent<TransformComponent>();
        auto* player = m_playerEntity.GetComponent<PlayerComponent>();
        if (transform && player) {
            playerPos = glm::vec2(transform->position);
            playerDirection = player->direction;
        }
    }
    
    switch(m_RenderMode) {
        case RenderMode::FOG: {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            FogConfig fogConfig;
            fogConfig.range = 500.0f;
            fogConfig.shadowSoftness = 0.4f;
            fogConfig.fogColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.9f);
            
            fogRenderer->DrawFogQuad(playerPos, fogConfig);
            break;
        }
        case RenderMode::VISION:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            visionRenderer->DrawVisionOverlay(playerPos, playerDirection, m_VisionConfig);
            break;
            
        case RenderMode::LIGHTING:
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            lightRenderer->DrawLightingOverlay(m_Lights, m_LightConfig);
            break;
    }
    
    glDisable(GL_BLEND);
    
    // Render UI - centralized ImGui frame handling
    RenderUI();
}

void Game::RenderUI() {
    // Only render UI if at least one system is initialized and visible
    bool shouldRenderUI = false;
    if (m_inspectorUI && m_inspectorUI->IsInitialized() && m_inspectorUI->IsVisible()) {
        shouldRenderUI = true;
    }
    if (m_networkUI && m_networkUI->IsInitialized() && m_networkUI->IsVisible()) {
        shouldRenderUI = true;
    }
    
    if (!shouldRenderUI) {
        return;
    }
    
    // Start ImGui frame (only once for all UI systems)
    if (m_inspectorUI && m_inspectorUI->IsInitialized()) {
        // Use the inspector's backend to start the frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Render Inspector UI content only
        if (m_inspectorUI->IsVisible()) {
            m_inspectorUI->RenderContent(m_scene.get());
            
            // Update renderers if any changes were made in the inspector
            UpdateRenderersFromECS();
        }
        
        // Render Network UI content only
        if (m_networkUI && m_networkUI->IsVisible()) {
            m_networkUI->RenderContent();
        }
        
        // End ImGui frame (only once for all UI systems)
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}

void Game::OnResize(int width, int height) {
    windowWidth = width;
    windowHeight = height;
    renderer->SetWindowSize(width, height);
    fogRenderer->SetWindowSize(width, height);
    visionRenderer->SetWindowSize(width, height);
    lightRenderer->SetWindowSize(width, height);
    
    // Update movement system window size
    if (m_playerMovementSystem) {
        m_playerMovementSystem->SetWindowSize(width, height);
    }
}

void Game::OnShutdown() {
    m_isShuttingDown = true;
    Logger::Info("Game Shutdown");
    
    // Save scene before shutdown
    if (m_scene) {
        m_scene->SaveToFile("autosave_scene.yaml");
        Logger::Info("Auto-saved scene to autosave_scene.yaml");
    }
    
    // Clean up network system first (before we destroy entities)
    if (Network::GetManager().IsClient()) {
        // Make sure we disconnect as a client if connected
        DisconnectFromServer();
    } else if (Network::GetManager().IsServer()) {
        // Stop server if running
        Network::GetManager().StopServer();
    }
    Network::Shutdown();
    Logger::Info("Network system shut down");
    
    // Clear network players first (to prevent access during shutdown)
    ClearNetworkPlayers();
    
    // Shutdown inspector
    if (m_inspectorUI) {
        m_inspectorUI->Shutdown();
        m_inspectorUI.reset();
    }
    
    // Shutdown network UI
    if (m_networkUI) {
        m_networkUI->Shutdown();
        m_networkUI.reset();
    }
    
    // Shutdown Audio System
    Audio::Shutdown();
    Logger::Info("Audio system shut down");
    
    // Add a small delay to ensure audio callbacks have completed
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Clean up renderers safely
    Logger::Info("Starting renderer cleanup...");
    
    try {
        if (renderer) {
            // Perform any renderer-specific cleanup before deletion
            Logger::Info("Cleaning up main renderer");
            delete renderer;
            renderer = nullptr;
        }
        
        if (fogRenderer) {
            Logger::Info("Cleaning up fog renderer");
            delete fogRenderer;
            fogRenderer = nullptr;
        }
        
        if (visionRenderer) {
            Logger::Info("Cleaning up vision renderer");
            delete visionRenderer;
            visionRenderer = nullptr;
        }
        
        if (lightRenderer) {
            Logger::Info("Cleaning up light renderer");
            delete lightRenderer;
            lightRenderer = nullptr;
        }
        
        Logger::Info("Renderers cleaned up successfully");
    }
    catch (const std::exception& e) {
        Logger::Error<Game>("Exception during renderer cleanup: " + std::string(e.what()), this);
    }
    catch (...) {
        Logger::Error<Game>("Unknown error during renderer cleanup", this);
    }
    
    Logger::Info("Shutdown complete");
}

// Legacy collision detection helper functions (kept for compatibility)
bool Game::CheckCollision(const Player& player, const Obstacle& obstacle) const {
    // Convert center-based positions to AABB bounds
    glm::vec2 playerMin = player.GetMinBounds();
    glm::vec2 playerMax = player.GetMaxBounds();
    
    glm::vec2 obstacleMin = obstacle.position - obstacle.size * 0.5f;
    glm::vec2 obstacleMax = obstacle.position + obstacle.size * 0.5f;
    
    // AABB collision detection
    return (playerMin.x < obstacleMax.x && playerMax.x > obstacleMin.x &&
            playerMin.y < obstacleMax.y && playerMax.y > obstacleMin.y);
}

glm::vec2 Game::ResolveCollision(const Player& player, const glm::vec2& newPos) const {
    glm::vec2 resolvedPos = newPos;
    
    // Create a temporary player with the new position for collision testing
    Player tempPlayer = player;
    tempPlayer.position = resolvedPos;
    
    // Check collision with all obstacles
    for (const auto& obstacle : m_Obstacles) {
        if (CheckCollision(tempPlayer, obstacle)) {
            // Calculate overlap and resolve collision
            glm::vec2 playerMin = tempPlayer.GetMinBounds();
            glm::vec2 playerMax = tempPlayer.GetMaxBounds();
            
            glm::vec2 obstacleMin = obstacle.position - obstacle.size * 0.5f;
            glm::vec2 obstacleMax = obstacle.position + obstacle.size * 0.5f;
            
            // Calculate overlap in both axes
            float overlapX = std::min(playerMax.x - obstacleMin.x, obstacleMax.x - playerMin.x);
            float overlapY = std::min(playerMax.y - obstacleMin.y, obstacleMax.y - playerMin.y);
            
            // Resolve collision by moving along the axis with minimum overlap
            if (overlapX < overlapY) {
                // Resolve horizontally
                if (resolvedPos.x < obstacle.position.x) {
                    // Player is to the left of obstacle
                    resolvedPos.x = obstacleMin.x - player.size.x * 0.5f;
                } else {
                    // Player is to the right of obstacle
                    resolvedPos.x = obstacleMax.x + player.size.x * 0.5f;
                }
            } else {
                // Resolve vertically
                if (resolvedPos.y < obstacle.position.y) {
                    // Player is above obstacle
                    resolvedPos.y = obstacleMin.y - player.size.y * 0.5f;
                } else {
                    // Player is below obstacle
                    resolvedPos.y = obstacleMax.y + player.size.y * 0.5f;
                }
            }
            
            // Update temp player position for subsequent collision checks
            tempPlayer.position = resolvedPos;
        }
    }
    
    return resolvedPos;
}

// Audio System Implementation
void Game::SetupAudioSystem() {
    Logger::Info("Initializing Audio System...");
    
    // Initialize the global audio manager
    if (!Audio::Initialize()) {
        Logger::Error<Game>("Failed to initialize Audio System", this);
        return;
    }
    
    // Set up audio event callback
    Audio::GetManager().SetEventCallback([this](const AudioEvent& event) {
        HandleAudioEvents(event);
    });
    
    // Load game audio assets
    LoadGameAudio();
    
    Logger::Info("Audio System initialized successfully");
}

void Game::LoadGameAudio() {
    Logger::Info("Loading game audio assets...");
    
    // Load sound effects using batch loading for efficiency
    std::vector<SoundAsset> soundEffects = {
        SoundAsset("gui_click", "resources/audio/sounds/gui/gui_click_7.mp3", 0.6f, 1.0f, 0.5f),
        SoundAsset("gui_check", "resources/audio/sounds/gui/gui_check_1.mp3", 0.6f, 1.0f, 0.5f),
        
        // Footstep sounds with different variations
        SoundAsset("footstep_concrete_1", "resources/audio/sounds/player/footsteps/concrete_1.mp3", 0.3f, 1.0f, 0.5f),
        SoundAsset("footstep_concrete_2", "resources/audio/sounds/player/footsteps/concrete_2.mp3", 0.3f, 1.0f, 0.5f),
        SoundAsset("footstep_concrete_3", "resources/audio/sounds/player/footsteps/concrete_3.mp3", 0.3f, 1.0f, 0.5f),
    };
    
    // Load background music
    std::vector<MusicAsset> backgroundMusic = {
        MusicAsset("game_music", "resources/audio/music/hope.ogg", true, 0.7f, 1.0f, 0.5f),
    };
    
    // Batch load all audio assets
    Audio::GetManager().LoadSoundBatch(soundEffects);
    Audio::GetManager().LoadMusicBatch(backgroundMusic);
    
    // Set initial master volume
    Audio::SetMasterVolume(0.7f);
    
    // Set up default player footsteps if needed
    if (m_scene) {
        auto entities = m_scene->GetEntitiesWith<PlayerComponent>();
        for (EntityID entityID : entities) {
            Entity entity(entityID, m_scene->GetEntityManager(), m_scene->GetComponentManager());
            auto* player = entity.GetComponent<PlayerComponent>();
            if (player) {
                // Only set up footsteps if they're not already set
                if (player->footsteps[0].name.empty()) {
                    player->footsteps[0] = SoundAsset("footstep_concrete_1", "resources/audio/sounds/player/footsteps/concrete_1.mp3", 0.3f, 1.0f, 0.5f);
                    player->footsteps[1] = SoundAsset("footstep_concrete_2", "resources/audio/sounds/player/footsteps/concrete_2.mp3", 0.3f, 1.0f, 0.5f);
                    player->footsteps[2] = SoundAsset("footstep_concrete_3", "resources/audio/sounds/player/footsteps/concrete_3.mp3", 0.3f, 1.0f, 0.5f);
                }
            }
        }
    }
    
    Logger::Info("Audio assets loading initiated...");
}

void Game::HandleAudioEvents(const AudioEvent& event) {
    switch (event.type) {
        case AudioEventType::SOUND_LOADED:
            Logger::Info("Sound loaded: " + event.soundName);
            break;
            
        case AudioEventType::SOUND_UNLOADED:
            Logger::Info("Sound unloaded: " + event.soundName);
            break;
            
        case AudioEventType::SOUND_STOPPED:
            Logger::Info("Sound stopped: " + event.soundName);
            // Reset footstep sound playing flag when footstep sounds stop
            if (m_scene && event.soundName.find("footstep") != std::string::npos) {
                auto entities = m_scene->GetEntitiesWith<PlayerComponent>();
                for (EntityID entityID : entities) {
                    Entity entity(entityID, m_scene->GetEntityManager(), m_scene->GetComponentManager());
                    auto* player = entity.GetComponent<PlayerComponent>();
                    if (player) {
                        for (int i = 0; i < 3; i++) {
                            if (player->footsteps[i].name == event.soundName) {
                                player->footsteps[i].isPlaying = false;
                                Logger::Info("Reset isPlaying flag for " + event.soundName);
                                break;
                            }
                        }
                    }
                }
            }
            break;
            
        case AudioEventType::MUSIC_LOADED:
            Logger::Info("Music loaded: " + event.soundName);
            // Auto-start background music when it's loaded
            if (event.soundName == "game_music") {
                Audio::PlayMusic("game_music", true);
                Logger::Info("Started background music");
            }
            break;
            
        case AudioEventType::MUSIC_STARTED:
            Logger::Info("Music started: " + event.soundName);
            break;
            
        case AudioEventType::MUSIC_FINISHED:
            Logger::Info("Music finished: " + event.soundName);
            break;
            
        case AudioEventType::AUDIO_ERROR:
            Logger::Error<Game>("Audio error for '" + event.soundName + "': " + event.message, this);
            break;
            
        default:
            break;
    }
}