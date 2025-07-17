#pragma once

#include <enet/enet.h>
#include <vector>
#include <string>
#include <cstring>
#include <glm/glm.hpp>

// Forward declarations
class NetworkManager;

// Packet types for game communication
enum class PacketType : uint8_t {
    // Connection management
    HANDSHAKE = 0, // Handshake packet
    DISCONNECT, // Disconnect packet
    PING, // Server to client message
    PONG, // Client to server response
    PEER_ID_ASSIGNMENT, // Server assigns peer ID to client
    
    
    // Player actions
    PLAYER_MOVE,
    PLAYER_POSITION_UPDATE,
    PLAYER_JOIN,
    PLAYER_LEAVE,
    
    // Game state
    GAME_STATE_UPDATE,
    ENTITY_SPAWN,
    ENTITY_DESTROY,
    ENTITY_UPDATE,
    
    // Chat/Communication
    CHAT_MESSAGE,
    
    // Custom game-specific packets
    CUSTOM_GAME_EVENT = 100
};

// Packet reliability modes
enum class PacketReliability {
    UNRELIABLE = 0,         // Fast, no guarantees
    RELIABLE = 1,           // Guaranteed delivery, ordered
    UNSEQUENCED = 2         // Fast, guaranteed but unordered
};

// Base packet structure
struct PacketHeader {
    PacketType type;
    uint32_t timestamp;
    uint32_t dataSize;
    
    PacketHeader() : type(PacketType::PING), timestamp(0), dataSize(0) {}
    PacketHeader(PacketType t, uint32_t size) 
        : type(t), timestamp(enet_time_get()), dataSize(size) {}
};

// Packet data container with serialization support
class Packet {
public:
    Packet() : m_Header(), m_Data() {}
    explicit Packet(PacketType type) : m_Header(type, 0), m_Data() {}
    
    // Header access
    const PacketHeader& GetHeader() const { return m_Header; }
    PacketType GetType() const { return m_Header.type; }
    uint32_t GetTimestamp() const { return m_Header.timestamp; }
    uint32_t GetDataSize() const { return m_Header.dataSize; }
    
    // Data writing
    void WriteUint8(uint8_t value);
    void WriteUint16(uint16_t value);
    void WriteUint32(uint32_t value);
    void WriteFloat(float value);
    void WriteString(const std::string& value);
    void WriteVec2(const glm::vec2& value);
    void WriteVec3(const glm::vec3& value);
    
    // Data reading
    uint8_t ReadUint8();
    uint16_t ReadUint16();
    uint32_t ReadUint32();
    float ReadFloat();
    std::string ReadString();
    glm::vec2 ReadVec2();
    glm::vec3 ReadVec3();
    
    // Raw data access
    const uint8_t* GetRawData() const;
    size_t GetTotalSize() const { return sizeof(PacketHeader) + m_Data.size(); }
    
    // Reset read position
    void ResetReadPosition() { m_ReadPos = 0; }
    
    // Create ENet packet from this packet
    ENetPacket* CreateENetPacket(PacketReliability reliability = PacketReliability::RELIABLE) const;
    
    // Create packet from ENet packet
    static Packet FromENetPacket(ENetPacket* enetPacket);
    
    // Clear packet data
    void Clear();

private:
    PacketHeader m_Header;
    std::vector<uint8_t> m_Data;
    mutable size_t m_ReadPos = 0;
    
    void UpdateHeader() { m_Header.dataSize = static_cast<uint32_t>(m_Data.size()); }
};

// Common packet structures for game events
namespace PacketData {
    
    // Player movement packet
    struct PlayerMove {
        uint32_t playerID;
        glm::vec2 position;
        glm::vec2 velocity;
        float rotation;
        
        void WriteTo(Packet& packet) const;
        void ReadFrom(Packet& packet);
    };
    
    // Chat message packet
    struct ChatMessage {
        uint32_t playerID;
        std::string playerName;
        std::string message;
        
        void WriteTo(Packet& packet) const;
        void ReadFrom(Packet& packet);
    };
    
    // Entity update packet
    struct EntityUpdate {
        uint32_t entityID;
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;
        bool isVisible;
        
        void WriteTo(Packet& packet) const;
        void ReadFrom(Packet& packet);
    };
    
    // Player join packet
    struct PlayerJoin {
        uint32_t playerID;
        std::string playerName;
        glm::vec2 spawnPosition;
        
        void WriteTo(Packet& packet) const;
        void ReadFrom(Packet& packet);
    };
    
    // Peer ID assignment packet
    struct PeerIDAssignment {
        uint32_t assignedPeerID;
        
        void WriteTo(Packet& packet) const;
        void ReadFrom(Packet& packet);
    };
}

// Packet factory for creating specific packet types
class PacketFactory {
public:
    static Packet CreatePingPacket();
    static Packet CreatePongPacket();
    static Packet CreatePlayerMovePacket(const PacketData::PlayerMove& moveData);
    static Packet CreateChatPacket(const PacketData::ChatMessage& chatData);
    static Packet CreateEntityUpdatePacket(const PacketData::EntityUpdate& entityData);
    static Packet CreatePlayerJoinPacket(const PacketData::PlayerJoin& joinData);
    static Packet CreateDisconnectPacket(const std::string& reason = "");
    static Packet CreatePlayerLeavePacket(uint32_t playerID);
    static Packet CreatePeerIDAssignmentPacket(uint32_t assignedPeerID);
};
