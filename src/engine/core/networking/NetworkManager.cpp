#include "NetworkManager.h"
#include <engine/utils/Logger.h>
#include <iostream>
#include <algorithm>

// Static error storage
std::string NetworkManager::s_LastError = "";

NetworkManager::NetworkManager()
    : m_Initialized(false)
    , m_IsServer(false)
    , m_IsClient(false)
    , m_Host(nullptr)
    , m_ServerPeer(nullptr)
    , m_NextPeerID(1)
    , m_EventCallback(nullptr)
    , m_BytesSent(0)
    , m_BytesReceived(0)
    , m_PacketsSent(0)
    , m_PacketsReceived(0)
    , m_MaxClients(32)
    , m_ChannelLimit(4)
    , m_IncomingBandwidth(0)
    , m_OutgoingBandwidth(0)
    , m_CompressionEnabled(false)
    , m_ThreadRunning(false)
    , m_PendingConnection(false)
{
    RegisterBuiltinHandlers();
}

NetworkManager::~NetworkManager() {
    Shutdown();
    
    // Stop network thread
    if (m_NetworkThread.joinable()) {
        m_ThreadRunning = false;
        m_ThreadCondition.notify_all();
        m_NetworkThread.join();
    }
}

bool NetworkManager::Initialize() {
    if (m_Initialized) {
        return true;
    }
    
    if (enet_initialize() != 0) {
        SetError("Failed to initialize ENet");
        return false;
    }
    
    // Start network thread
    m_ThreadRunning = true;
    m_NetworkThread = std::thread(&NetworkManager::NetworkThreadFunction, this);
    
    SetupDefaultHandlers();
    m_Initialized = true;
    
    Logger::Info("NetworkManager initialized successfully");
    return true;
}

void NetworkManager::Shutdown() {
    if (!m_Initialized) {
        return;
    }
    
    // Stop network thread first
    if (m_ThreadRunning) {
        m_ThreadRunning = false;
        m_ThreadCondition.notify_all();
        if (m_NetworkThread.joinable()) {
            m_NetworkThread.join();
        }
    }
    
    StopServer();
    
    if (IsClient()) {
        DisconnectFromServer("Shutting down");
    }
    
    if (m_Host) {
        enet_host_destroy(m_Host);
        m_Host = nullptr;
    }
    
    m_ConnectedPeers.clear();
    m_PacketHandlers.clear();
    
    enet_deinitialize();
    m_Initialized = false;
    
    Logger::Info("NetworkManager shut down");
}

bool NetworkManager::StartServer(uint16_t port, size_t maxClients) {
    if (!m_Initialized) {
        SetError("NetworkManager not initialized");
        return false;
    }
    
    if (m_IsServer || m_IsClient) {
        SetError("Already running as server or client");
        return false;
    }
    
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    
    m_MaxClients = maxClients;
    m_Host = enet_host_create(&address, maxClients, m_ChannelLimit, 
                             m_IncomingBandwidth, m_OutgoingBandwidth);
    
    if (!m_Host) {
        SetError("Failed to create server host on port " + std::to_string(port));
        return false;
    }
    
    if (m_CompressionEnabled) {
        enet_host_compress_with_range_coder(m_Host);
    }
    
    m_IsServer = true;
    m_LocalPeerID = 0; // Server always has peer ID 0
    m_ConnectedPeers.clear();
    
    Logger::Info("Server started on port " + std::to_string(port) + 
                " with max " + std::to_string(maxClients) + " clients");
    Logger::Info("Server assigned local peer ID: 0");
    
    // Queue server started event
    QueueEvent(NetworkEvent(NetworkEventType::SERVER_STARTED, 0, 
                           "Server started on port " + std::to_string(port)));
    
    return true;
}

void NetworkManager::StopServer() {
    if (!m_IsServer || !m_Host) {
        return;
    }
    
    // Disconnect all clients
    for (auto& peer : m_ConnectedPeers) {
        if (peer.enetPeer && peer.isConnected) {
            enet_peer_disconnect(peer.enetPeer, 0);
        }
    }
    
    // Process final events
    ENetEvent event;
    while (enet_host_service(m_Host, &event, 100) > 0) {
        if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            RemovePeer(event.peer);
        }
    }
    
    enet_host_destroy(m_Host);
    m_Host = nullptr;
    m_IsServer = false;
    m_ConnectedPeers.clear();
    
    Logger::Info("Server stopped");
    QueueEvent(NetworkEvent(NetworkEventType::SERVER_STOPPED));
}

bool NetworkManager::ConnectToServer(const std::string& address, uint16_t port, uint32_t timeoutMs) {
    if (!m_Initialized) {
        SetError("NetworkManager not initialized");
        return false;
    }
    
    if (m_IsServer || m_IsClient) {
        SetError("Already running as server or client");
        return false;
    }
    
    if (m_PendingConnection) {
        SetError("Connection already in progress");
        return false;
    }
    
    // Set up async connection data
    {
        std::lock_guard<std::mutex> lock(m_ConnectionDataMutex);
        m_ConnectionData.address = address;
        m_ConnectionData.port = port;
        m_ConnectionData.timeoutMs = timeoutMs;
    }
    
    // Signal the network thread to start connection
    m_PendingConnection = true;
    m_ThreadCondition.notify_one();
    
    Logger::Info("Starting async connection to " + address + ":" + std::to_string(port));
    return true; // Return true for async operation start
}

bool NetworkManager::ConnectToServerBlocking(const std::string& address, uint16_t port, uint32_t timeoutMs) {
    // This is the original blocking implementation for use in the network thread
    
    m_Host = enet_host_create(nullptr, 1, m_ChannelLimit, 
                             m_IncomingBandwidth, m_OutgoingBandwidth);
    
    if (!m_Host) {
        SetError("Failed to create client host");
        return false;
    }
    
    if (m_CompressionEnabled) {
        enet_host_compress_with_range_coder(m_Host);
    }
    
    ENetAddress serverAddress;
    if (enet_address_set_host(&serverAddress, address.c_str()) != 0) {
        enet_host_destroy(m_Host);
        m_Host = nullptr;
        SetError("Invalid server address: " + address);
        return false;
    }
    serverAddress.port = port;
    
    m_ServerPeer = enet_host_connect(m_Host, &serverAddress, m_ChannelLimit, 0);
    if (!m_ServerPeer) {
        enet_host_destroy(m_Host);
        m_Host = nullptr;
        SetError("Failed to create connection to server");
        return false;
    }
    
    // Wait for connection result
    ENetEvent event;
    if (enet_host_service(m_Host, &event, timeoutMs) > 0 && 
        event.type == ENET_EVENT_TYPE_CONNECT) {
        
        m_IsClient = true;
        
        // Add server as a peer
        PeerInfo serverInfo;
        serverInfo.id = 0; // Server always gets ID 0
        serverInfo.enetPeer = m_ServerPeer;
        serverInfo.address = address;
        serverInfo.port = port;
        serverInfo.isConnected = true;
        m_ConnectedPeers.push_back(serverInfo);
        
        // Don't assign our own client ID yet - wait for server to send it via PEER_ID_ASSIGNMENT packet
        m_LocalPeerID = 0; // Will be set when we receive the assignment packet
        
        Logger::Info("Connected to server " + address + ":" + std::to_string(port) + ", waiting for peer ID assignment");
        // Note: We'll queue the SERVER_CONNECTED event after receiving peer ID assignment
        return true;
    } else {
        enet_peer_reset(m_ServerPeer);
        enet_host_destroy(m_Host);
        m_Host = nullptr;
        m_ServerPeer = nullptr;
        SetError("Connection to server timed out");
        QueueEvent(NetworkEvent(NetworkEventType::CONNECTION_FAILED, 0, "Connection timed out"));
        return false;
    }
}

void NetworkManager::DisconnectFromServer(const std::string& reason) {
    if (!m_IsClient || !m_ServerPeer) {
        return;
    }
    
    // Send disconnect packet with reason
    if (!reason.empty()) {
        Packet disconnectPacket = PacketFactory::CreateDisconnectPacket(reason);
        SendPacket(disconnectPacket, 0, PacketReliability::RELIABLE);
    }
    
    enet_peer_disconnect(m_ServerPeer, 0);
    
    // Wait for disconnect acknowledgment
    ENetEvent event;
    while (enet_host_service(m_Host, &event, 3000) > 0) {
        if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            break;
        }
    }
    
    enet_host_destroy(m_Host);
    m_Host = nullptr;
    m_ServerPeer = nullptr;
    m_IsClient = false;
    m_ConnectedPeers.clear();
    
    Logger::Info("Disconnected from server: " + reason);
    
    // Instead of just queuing the event, immediately trigger the callback
    // This ensures cleanup happens before network thread shutdown
    NetworkEvent disconnectEvent(NetworkEventType::SERVER_DISCONNECTED, 0, reason);
    QueueEvent(disconnectEvent);
    
    // CRITICAL: Immediately trigger the event callback if it exists
    if (m_EventCallback) {
        Logger::Info("Immediately triggering SERVER_DISCONNECTED callback...");
        m_EventCallback(disconnectEvent);
    }
}

bool NetworkManager::IsConnectedToServer() const {
    return m_IsClient && m_ServerPeer && 
           m_ServerPeer->state == ENET_PEER_STATE_CONNECTED;
}

bool NetworkManager::SendPacket(const Packet& packet, uint32_t peerID, 
                                PacketReliability reliability, uint8_t channel) {
    if (!m_Host) {
        SetError("No active host");
        return false;
    }
    
    ENetPacket* enetPacket = packet.CreateENetPacket(reliability);
    if (!enetPacket) {
        SetError("Failed to create ENet packet");
        return false;
    }
    
    bool success = false;
    
    if (m_IsClient) {
        // Client sends to server (peerID is ignored)
        if (m_ServerPeer && m_ServerPeer->state == ENET_PEER_STATE_CONNECTED) {
            success = enet_peer_send(m_ServerPeer, channel, enetPacket) == 0;
        }
    } else if (m_IsServer) {
        // Server sends to specific client
        PeerInfo* peer = GetPeerInfo(peerID);
        if (peer && peer->enetPeer && peer->isConnected) {
            success = enet_peer_send(peer->enetPeer, channel, enetPacket) == 0;
        }
    }
    
    if (success) {
        m_BytesSent += packet.GetTotalSize();
        m_PacketsSent++;
    } else {
        enet_packet_destroy(enetPacket);
        SetError("Failed to send packet");
    }
    
    return success;
}

bool NetworkManager::BroadcastPacket(const Packet& packet, 
                                    PacketReliability reliability, uint8_t channel) {
    if (!m_IsServer || !m_Host) {
        SetError("Not running as server");
        return false;
    }
    
    ENetPacket* enetPacket = packet.CreateENetPacket(reliability);
    if (!enetPacket) {
        SetError("Failed to create ENet packet");
        return false;
    }
    
    enet_host_broadcast(m_Host, channel, enetPacket);
    
    m_BytesSent += packet.GetTotalSize() * m_ConnectedPeers.size();
    m_PacketsSent++;
    
    return true;
}

void NetworkManager::Update() {
    if (!m_Host) {
        return;
    }
    
    ProcessEvents();
    
    // Send periodic pings
    static uint32_t lastPingTime = 0;
    uint32_t currentTime = enet_time_get();
    
    if (currentTime - lastPingTime > 5000) { // Ping every 5 seconds
        for (auto& peer : m_ConnectedPeers) {
            if (peer.isConnected) {
                SendPing(peer.id);
            }
        }
        lastPingTime = currentTime;
    }
    
    // Process queued events (thread-safe)
    std::queue<NetworkEvent> eventsToProcess;
    {
        std::lock_guard<std::mutex> lock(m_EventQueueMutex);
        eventsToProcess = std::move(m_EventQueue);
        m_EventQueue = std::queue<NetworkEvent>(); // Clear the queue
    }
    
    while (!eventsToProcess.empty()) {
        if (m_EventCallback) {
            m_EventCallback(eventsToProcess.front());
        }
        eventsToProcess.pop();
    }
}

void NetworkManager::ProcessEvents() {
    ENetEvent event;
    
    while (enet_host_service(m_Host, &event, 0) > 0) {
        HandleENetEvent(event);
    }
}

void NetworkManager::HandleENetEvent(const ENetEvent& event) {
    switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT: {
            if (m_IsServer) {
                AddPeer(event.peer);
                Logger::Info("Client connected from " + 
                           std::to_string((event.peer->address.host & 0xFF)) + "." +
                           std::to_string(((event.peer->address.host >> 8) & 0xFF)) + "." +
                           std::to_string(((event.peer->address.host >> 16) & 0xFF)) + "." +
                           std::to_string(((event.peer->address.host >> 24) & 0xFF)) + ":" +
                           std::to_string(event.peer->address.port));
            }
            break;
        }
        
        case ENET_EVENT_TYPE_DISCONNECT: {
            if (m_IsServer) {
                PeerInfo* peer = FindPeerByENetPeer(event.peer);
                if (peer) {
                    QueueEvent(NetworkEvent(NetworkEventType::CLIENT_DISCONNECTED, 
                                          peer->id, "Client disconnected"));
                    Logger::Info("Client " + std::to_string(peer->id) + " disconnected");
                    RemovePeer(event.peer);
                }
            } else if (m_IsClient) {
                QueueEvent(NetworkEvent(NetworkEventType::SERVER_DISCONNECTED, 0, 
                                      "Server disconnected"));
                Logger::Info("Server disconnected");
            }
            break;
        }
        
        case ENET_EVENT_TYPE_RECEIVE: {
            try {
                Packet packet = Packet::FromENetPacket(event.packet);
                
                m_BytesReceived += event.packet->dataLength;
                m_PacketsReceived++;
                
                // Find sender peer ID
                uint32_t senderID = 0;
                if (m_IsServer) {
                    PeerInfo* peer = FindPeerByENetPeer(event.peer);
                    if (peer) {
                        senderID = peer->id;
                    }
                }
                
                // Handle packet through registered handlers
                auto handler = m_PacketHandlers.find(packet.GetType());
                if (handler != m_PacketHandlers.end()) {
                    handler->second(packet, senderID);
                } else {
                    // Queue as generic packet received event
                    NetworkEvent netEvent(NetworkEventType::PACKET_RECEIVED, senderID);
                    netEvent.packet = packet;
                    QueueEvent(netEvent);
                }
                
            } catch (const std::exception& e) {
                Logger::Error<NetworkManager>("Failed to process received packet: " + 
                                            std::string(e.what()), this);
            }
            
            enet_packet_destroy(event.packet);
            break;
        }
        
        default:
            break;
    }
}

void NetworkManager::RegisterPacketHandler(PacketType type, PacketHandler handler) {
    m_PacketHandlers[type] = handler;
}

void NetworkManager::UnregisterPacketHandler(PacketType type) {
    m_PacketHandlers.erase(type);
}

void NetworkManager::AddPeer(ENetPeer* enetPeer) {
    PeerInfo peer;
    peer.id = m_NextPeerID++;
    peer.enetPeer = enetPeer;
    peer.isConnected = true;
    
    // Set peer data to our peer ID for easy lookup
    enetPeer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(peer.id));
    
    // Get address info
    char hostBuffer[256];
    if (enet_address_get_host_ip(&enetPeer->address, hostBuffer, sizeof(hostBuffer)) == 0) {
        peer.address = hostBuffer;
    }
    peer.port = enetPeer->address.port;
    
    m_ConnectedPeers.push_back(peer);
    
    // Send peer ID assignment to the newly connected client
    Logger::Info("=== SERVER ASSIGNING PEER ID ===");
    Logger::Info("Assigning peer ID: " + std::to_string(peer.id) + " to new client");
    Logger::Info("Server local peer ID: " + std::to_string(m_LocalPeerID));
    
    Packet peerIDPacket = PacketFactory::CreatePeerIDAssignmentPacket(peer.id);
    ENetPacket* enetPacket = peerIDPacket.CreateENetPacket(PacketReliability::RELIABLE);
    if (enetPacket && enet_peer_send(enetPeer, 0, enetPacket) == 0) {
        Logger::Info("Successfully sent PEER_ID_ASSIGNMENT packet to client (assigned ID: " + std::to_string(peer.id) + ")");
        enet_host_flush(m_Host); // Force immediate send
    } else {
        Logger::Error<NetworkManager>("Failed to send peer ID assignment packet", this);
        if (enetPacket) enet_packet_destroy(enetPacket);
    }
    
    QueueEvent(NetworkEvent(NetworkEventType::CLIENT_CONNECTED, peer.id, 
                           "Client " + std::to_string(peer.id) + " connected"));
}

void NetworkManager::RemovePeer(ENetPeer* enetPeer) {
    auto it = std::find_if(m_ConnectedPeers.begin(), m_ConnectedPeers.end(),
                          [enetPeer](const PeerInfo& peer) {
                              return peer.enetPeer == enetPeer;
                          });
    
    if (it != m_ConnectedPeers.end()) {
        m_ConnectedPeers.erase(it);
    }
}

PeerInfo* NetworkManager::FindPeerByENetPeer(ENetPeer* enetPeer) {
    auto it = std::find_if(m_ConnectedPeers.begin(), m_ConnectedPeers.end(),
                          [enetPeer](PeerInfo& peer) {
                              return peer.enetPeer == enetPeer;
                          });
    
    return (it != m_ConnectedPeers.end()) ? &(*it) : nullptr;
}

PeerInfo* NetworkManager::GetPeerInfo(uint32_t peerID) {
    auto it = std::find_if(m_ConnectedPeers.begin(), m_ConnectedPeers.end(),
                          [peerID](PeerInfo& peer) {
                              return peer.id == peerID;
                          });
    
    return (it != m_ConnectedPeers.end()) ? &(*it) : nullptr;
}

const PeerInfo* NetworkManager::GetPeerInfo(uint32_t peerID) const {
    auto it = std::find_if(m_ConnectedPeers.begin(), m_ConnectedPeers.end(),
                          [peerID](const PeerInfo& peer) {
                              return peer.id == peerID;
                          });
    
    return (it != m_ConnectedPeers.end()) ? &(*it) : nullptr;
}
void NetworkManager::QueueEvent(const NetworkEvent& event) {
    std::lock_guard<std::mutex> lock(m_EventQueueMutex);
    m_EventQueue.push(event);
}

void NetworkManager::SetError(const std::string& error) {
    s_LastError = error;
    Logger::Error<NetworkManager>(error, this);
}

uint32_t NetworkManager::GetLatency(uint32_t peerID) const {
    if (m_IsClient && peerID == 0 && m_ServerPeer) {
        return m_ServerPeer->roundTripTime;
    } else if (m_IsServer) {
        const PeerInfo* peer = GetPeerInfo(peerID);
        if (peer && peer->enetPeer) {
            return peer->enetPeer->roundTripTime;
        }
    }
    return 0;
}

void NetworkManager::SetChannelLimit(size_t limit) {
    m_ChannelLimit = limit;
    if (m_Host) {
        enet_host_channel_limit(m_Host, limit);
    }
}

void NetworkManager::SetBandwidthLimit(uint32_t incomingBandwidth, uint32_t outgoingBandwidth) {
    m_IncomingBandwidth = incomingBandwidth;
    m_OutgoingBandwidth = outgoingBandwidth;
    if (m_Host) {
        enet_host_bandwidth_limit(m_Host, incomingBandwidth, outgoingBandwidth);
    }
}

void NetworkManager::SetCompressionEnabled(bool enabled) {
    m_CompressionEnabled = enabled;
    if (m_Host && enabled) {
        enet_host_compress_with_range_coder(m_Host);
    }
}

// Ping-pong system
// Server sends ping to client
void NetworkManager::SendPing(uint32_t peerID) {
    Packet pingPacket = PacketFactory::CreatePingPacket();
    SendPacket(pingPacket, peerID, PacketReliability::UNRELIABLE);
}
// Client responds with pong
// latency is delay of client response
void NetworkManager::SendPong(uint32_t peerID) {
    Packet pongPacket = PacketFactory::CreatePongPacket();
    SendPacket(pongPacket, peerID, PacketReliability::UNRELIABLE);
}

void NetworkManager::UpdatePeerLatency(uint32_t peerID, uint32_t timestamp) {
    uint32_t currentTime = enet_time_get();
    uint32_t roundTripTime = currentTime - timestamp;
    
    PeerInfo* peer = GetPeerInfo(peerID);
    if (peer) {
        peer->roundTripTime = roundTripTime;
        peer->lastPingTime = currentTime;
    }
}

void NetworkManager::HandlePing(const Packet& packet, uint32_t peerID) {
    // Respond with pong
    Packet pongPacket = PacketFactory::CreatePongPacket();
    SendPacket(pongPacket, peerID, PacketReliability::UNRELIABLE);
}

void NetworkManager::HandlePong(const Packet& packet, uint32_t peerID) {
    // Update peer ping time
    PeerInfo* peer = GetPeerInfo(peerID);
    if (peer) {
        peer->roundTripTime = enet_time_get() - peer->lastPingTime;
    }
}

void NetworkManager::SetupDefaultHandlers() {
    RegisterBuiltinHandlers();
}

void NetworkManager::RegisterBuiltinHandlers() {
    // Register built-in packet handlers
    RegisterPacketHandler(PacketType::PING,
        [this](const Packet& packet, uint32_t senderID) {
            SendPong(senderID);
        });
    
    RegisterPacketHandler(PacketType::PONG,
        [this](const Packet& packet, uint32_t senderID) {
            UpdatePeerLatency(senderID, packet.GetTimestamp());
        });
    
        RegisterPacketHandler(PacketType::PEER_ID_ASSIGNMENT,
        [this](const Packet& packet, uint32_t senderID) {
            if (m_IsClient) {
                Packet mutablePacket = packet;
                uint32_t assignedID = mutablePacket.ReadUint32();
                uint32_t oldID = m_LocalPeerID;
                m_LocalPeerID = assignedID;
                
                Logger::Info("=== PEER ID ASSIGNMENT ===");
                Logger::Info("Received peer ID assignment: " + std::to_string(assignedID) + 
                           " (old ID was: " + std::to_string(oldID) + ")");
                Logger::Info("NetworkManager.GetLocalPeerID() now returns: " + std::to_string(m_LocalPeerID));
                
                // Now queue the SERVER_CONNECTED event with the correct peer ID
                QueueEvent(NetworkEvent(NetworkEventType::SERVER_CONNECTED, assignedID, 
                                      "Connected with assigned peer ID " + std::to_string(assignedID)));
            } else {
                Logger::Warn<NetworkManager>("Received PEER_ID_ASSIGNMENT packet but we're not a client (senderID: " + 
                                           std::to_string(senderID) + ")", this);
            }
        });
}

void NetworkManager::NetworkThreadFunction() {
    Logger::Info("Network thread started");
    
    while (m_ThreadRunning) {
        // Check for pending connection requests
        if (m_PendingConnection) {
            AsyncConnectionData connectionData;
            {
                std::lock_guard<std::mutex> lock(m_ConnectionDataMutex);
                connectionData = m_ConnectionData;
            }
            
            Logger::Info("Network thread: Attempting connection to " + 
                        connectionData.address + ":" + std::to_string(connectionData.port));
            
            bool success = ConnectToServerBlocking(connectionData.address, 
                                                 connectionData.port, 
                                                 connectionData.timeoutMs);
            
            if (!success) {
                Logger::Error<NetworkManager>("Failed to connect to server: " + GetLastError(), this);
            }
            
            m_PendingConnection = false;
        }
        
        // Wait for next operation or timeout
        std::unique_lock<std::mutex> lock(m_EventQueueMutex);
        m_ThreadCondition.wait_for(lock, std::chrono::milliseconds(100));
    }
    
    Logger::Info("Network thread stopped");
}

// Global Network namespace implementation
namespace Network {
    static std::unique_ptr<NetworkManager> g_NetworkManager = nullptr;
    
    NetworkManager& GetManager() {
        if (!g_NetworkManager) {
            g_NetworkManager = std::make_unique<NetworkManager>();
        }
        return *g_NetworkManager;
    }
    
    bool Initialize() {
        return GetManager().Initialize();
    }
    
    void Shutdown() {
        if (g_NetworkManager) {
            g_NetworkManager->Shutdown();
            g_NetworkManager.reset();
        }
    }
} 