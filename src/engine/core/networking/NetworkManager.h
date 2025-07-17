#pragma once

#include "Packet.h"
#include <enet/enet.h>
#include <functional>
#include <queue>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

// Network events
enum class NetworkEventType {
    CLIENT_CONNECTED,
    CLIENT_DISCONNECTED,
    SERVER_CONNECTED,
    SERVER_DISCONNECTED,
    PACKET_RECEIVED,
    CONNECTION_FAILED,
    SERVER_STARTED,
    SERVER_STOPPED
};

// Network event data
struct NetworkEvent {
    NetworkEventType type;
    uint32_t peerID;          // ID of the peer involved
    std::string message;      // Optional message (for errors, disconnect reasons, etc.)
    Packet packet;           // Packet data (if applicable)
    
    NetworkEvent(NetworkEventType t, uint32_t id = 0, const std::string& msg = "")
        : type(t), peerID(id), message(msg) {}
};

// Peer information
struct PeerInfo {
    uint32_t id;
    ENetPeer* enetPeer;
    std::string address;
    uint16_t port;
    uint32_t lastPingTime;
    uint32_t roundTripTime;
    bool isConnected;
    
    PeerInfo() : id(0), enetPeer(nullptr), port(0), lastPingTime(0), roundTripTime(0), isConnected(false) {}
};

// Network callback types
using NetworkEventCallback = std::function<void(const NetworkEvent&)>;
using PacketHandler = std::function<void(const Packet&, uint32_t peerID)>;

// Main NetworkManager class
class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();
    
    // Initialization and cleanup
    bool Initialize();
    void Shutdown();
    bool IsInitialized() const { return m_Initialized; }
    
    // Server functions
    bool StartServer(uint16_t port, size_t maxClients = 32);
    void StopServer();
    bool IsServer() const { return m_IsServer && m_Host != nullptr; }
    
    // Client functions
    bool ConnectToServer(const std::string& address, uint16_t port, uint32_t timeoutMs = 5000);
    void DisconnectFromServer(const std::string& reason = "");
    bool IsClient() const { return m_IsClient && m_Host != nullptr; }
    bool IsConnectedToServer() const;
    
    // Packet sending
    bool SendPacket(const Packet& packet, uint32_t peerID = 0, 
                    PacketReliability reliability = PacketReliability::RELIABLE, 
                    uint8_t channel = 0);
    bool BroadcastPacket(const Packet& packet, 
                         PacketReliability reliability = PacketReliability::RELIABLE, 
                         uint8_t channel = 0);
    
    // Network update (call this every frame)
    void Update();
    
    // Event handling
    void SetEventCallback(NetworkEventCallback callback) { m_EventCallback = callback; }
    void RegisterPacketHandler(PacketType type, PacketHandler handler);
    void UnregisterPacketHandler(PacketType type);
    
    // Peer management
    const std::vector<PeerInfo>& GetConnectedPeers() const { return m_ConnectedPeers; }
    PeerInfo* GetPeerInfo(uint32_t peerID);
    const PeerInfo* GetPeerInfo(uint32_t peerID) const;
    size_t GetPeerCount() const { return m_ConnectedPeers.size(); }
    uint32_t GetLocalPeerID() const { return m_LocalPeerID; }

    // Statistics
    uint32_t GetLatency(uint32_t peerID = 0) const;
    uint64_t GetBytesSent() const { return m_BytesSent; }
    uint64_t GetBytesReceived() const { return m_BytesReceived; }
    uint32_t GetPacketsSent() const { return m_PacketsSent; }
    uint32_t GetPacketsReceived() const { return m_PacketsReceived; }
    
    // Configuration
    void SetChannelLimit(size_t limit);
    void SetBandwidthLimit(uint32_t incomingBandwidth, uint32_t outgoingBandwidth);
    void SetCompressionEnabled(bool enabled);
    
    // Utility
    static std::string GetLastError() { return s_LastError; }
    
private:
    // Internal state
    bool m_Initialized;
    bool m_IsServer;
    bool m_IsClient;
    ENetHost* m_Host;
    ENetPeer* m_ServerPeer; // For client: connection to server
    
    // Peer management
    std::vector<PeerInfo> m_ConnectedPeers;
    uint32_t m_NextPeerID;
    uint32_t m_LocalPeerID;
    
    // Event handling
    NetworkEventCallback m_EventCallback;
    std::unordered_map<PacketType, PacketHandler> m_PacketHandlers;
    std::queue<NetworkEvent> m_EventQueue;
    
    // Statistics
    uint64_t m_BytesSent;
    uint64_t m_BytesReceived;
    uint32_t m_PacketsSent;
    uint32_t m_PacketsReceived;
    
    // Configuration
    size_t m_MaxClients;
    size_t m_ChannelLimit;
    uint32_t m_IncomingBandwidth;
    uint32_t m_OutgoingBandwidth;
    bool m_CompressionEnabled;
    
    // Error handling
    static std::string s_LastError;
    
    // Threading support
    std::thread m_NetworkThread;
    std::atomic<bool> m_ThreadRunning;
    std::mutex m_EventQueueMutex;
    std::condition_variable m_ThreadCondition;
    std::atomic<bool> m_PendingConnection;
    
    // Async connection data
    struct AsyncConnectionData {
        std::string address;
        uint16_t port;
        uint32_t timeoutMs;
    };
    std::mutex m_ConnectionDataMutex;
    AsyncConnectionData m_ConnectionData;
    
    // Internal methods
    void ProcessEvents();
    void HandleENetEvent(const ENetEvent& event);
    void NetworkThreadFunction(); // New thread function
    bool ConnectToServerBlocking(const std::string& address, uint16_t port, uint32_t timeoutMs); // Blocking version
    void RegisterBuiltinHandlers(); // Register built-in packet handlers
    void SendPong(uint32_t peerID); // Send pong packet
    void UpdatePeerLatency(uint32_t peerID, uint32_t timestamp); // Update peer latency
    void AddPeer(ENetPeer* enetPeer);
    void RemovePeer(ENetPeer* enetPeer);
    PeerInfo* FindPeerByENetPeer(ENetPeer* enetPeer);
    void QueueEvent(const NetworkEvent& event);
    void SetError(const std::string& error);
    
    // Ping system
    void SendPing(uint32_t peerID);
    void HandlePing(const Packet& packet, uint32_t peerID);
    void HandlePong(const Packet& packet, uint32_t peerID);
    
    // Built-in packet handlers
    void SetupDefaultHandlers();
};

// Global network manager instance (optional singleton pattern)
namespace Network {
    NetworkManager& GetManager();
    bool Initialize();
    void Shutdown();
    
    // Convenience functions
    inline bool StartServer(uint16_t port, size_t maxClients = 32) {
        return GetManager().StartServer(port, maxClients);
    }
    
    inline bool ConnectToServer(const std::string& address, uint16_t port) {
        return GetManager().ConnectToServer(address, port);
    }
    
    inline bool SendPacket(const Packet& packet, uint32_t peerID = 0) {
        return GetManager().SendPacket(packet, peerID);
    }
    
    inline bool BroadcastPacket(const Packet& packet) {
        return GetManager().BroadcastPacket(packet);
    }
    
    inline void Update() {
        GetManager().Update();
    }
} 