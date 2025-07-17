#include "Packet.h"
#include <stdexcept>

// Packet class implementation

void Packet::WriteUint8(uint8_t value) {
    m_Data.push_back(value);
    UpdateHeader();
}

void Packet::WriteUint16(uint16_t value) {
    m_Data.push_back(static_cast<uint8_t>(value & 0xFF));
    m_Data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    UpdateHeader();
}

void Packet::WriteUint32(uint32_t value) {
    m_Data.push_back(static_cast<uint8_t>(value & 0xFF));
    m_Data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    m_Data.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    m_Data.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    UpdateHeader();
}

void Packet::WriteFloat(float value) {
    union { float f; uint32_t i; } converter;
    converter.f = value;
    WriteUint32(converter.i);
}

void Packet::WriteString(const std::string& value) {
    WriteUint16(static_cast<uint16_t>(value.length()));
    for (char c : value) {
        WriteUint8(static_cast<uint8_t>(c));
    }
}

void Packet::WriteVec2(const glm::vec2& value) {
    WriteFloat(value.x);
    WriteFloat(value.y);
}

void Packet::WriteVec3(const glm::vec3& value) {
    WriteFloat(value.x);
    WriteFloat(value.y);
    WriteFloat(value.z);
}

uint8_t Packet::ReadUint8() {
    if (m_ReadPos >= m_Data.size()) {
        throw std::runtime_error("Packet read overflow: tried to read past end of data");
    }
    return m_Data[m_ReadPos++];
}

uint16_t Packet::ReadUint16() {
    uint16_t value = 0;
    value |= static_cast<uint16_t>(ReadUint8());
    value |= static_cast<uint16_t>(ReadUint8()) << 8;
    return value;
}

uint32_t Packet::ReadUint32() {
    uint32_t value = 0;
    value |= static_cast<uint32_t>(ReadUint8());
    value |= static_cast<uint32_t>(ReadUint8()) << 8;
    value |= static_cast<uint32_t>(ReadUint8()) << 16;
    value |= static_cast<uint32_t>(ReadUint8()) << 24;
    return value;
}

float Packet::ReadFloat() {
    union { float f; uint32_t i; } converter;
    converter.i = ReadUint32();
    return converter.f;
}

std::string Packet::ReadString() {
    uint16_t length = ReadUint16();
    std::string result;
    result.reserve(length);
    for (uint16_t i = 0; i < length; ++i) {
        result.push_back(static_cast<char>(ReadUint8()));
    }
    return result;
}

glm::vec2 Packet::ReadVec2() {
    float x = ReadFloat();
    float y = ReadFloat();
    return glm::vec2(x, y);
}

glm::vec3 Packet::ReadVec3() {
    float x = ReadFloat();
    float y = ReadFloat();
    float z = ReadFloat();
    return glm::vec3(x, y, z);
}

const uint8_t* Packet::GetRawData() const {
    static std::vector<uint8_t> combined;
    combined.clear();
    
    // Add header
    const uint8_t* headerBytes = reinterpret_cast<const uint8_t*>(&m_Header);
    combined.insert(combined.end(), headerBytes, headerBytes + sizeof(PacketHeader));
    
    // Add data
    combined.insert(combined.end(), m_Data.begin(), m_Data.end());
    
    return combined.data();
}

ENetPacket* Packet::CreateENetPacket(PacketReliability reliability) const {
    uint32_t flags = 0;
    
    switch (reliability) {
        case PacketReliability::RELIABLE:
            flags = ENET_PACKET_FLAG_RELIABLE;
            break;
        case PacketReliability::UNRELIABLE:
            flags = 0;
            break;
        case PacketReliability::UNSEQUENCED:
            flags = ENET_PACKET_FLAG_UNSEQUENCED;
            break;
    }
    
    return enet_packet_create(GetRawData(), GetTotalSize(), flags);
}

Packet Packet::FromENetPacket(ENetPacket* enetPacket) {
    if (!enetPacket || enetPacket->dataLength < sizeof(PacketHeader)) {
        throw std::runtime_error("Invalid ENet packet: too small or null");
    }
    
    Packet packet;
    
    // Read header
    std::memcpy(&packet.m_Header, enetPacket->data, sizeof(PacketHeader));
    
    // Read data
    size_t dataSize = enetPacket->dataLength - sizeof(PacketHeader);
    if (dataSize > 0) {
        packet.m_Data.resize(dataSize);
        std::memcpy(packet.m_Data.data(), 
                   static_cast<uint8_t*>(enetPacket->data) + sizeof(PacketHeader), 
                   dataSize);
    }
    
    packet.m_ReadPos = 0;
    return packet;
}

void Packet::Clear() {
    m_Data.clear();
    m_ReadPos = 0;
    m_Header = PacketHeader();
}

// PacketData implementations

void PacketData::PlayerMove::WriteTo(Packet& packet) const {
    packet.WriteUint32(playerID);
    packet.WriteVec2(position);
    packet.WriteVec2(velocity);
    packet.WriteFloat(rotation);
}

void PacketData::PlayerMove::ReadFrom(Packet& packet) {
    playerID = packet.ReadUint32();
    position = packet.ReadVec2();
    velocity = packet.ReadVec2();
    rotation = packet.ReadFloat();
}

void PacketData::ChatMessage::WriteTo(Packet& packet) const {
    packet.WriteUint32(playerID);
    packet.WriteString(playerName);
    packet.WriteString(message);
}

void PacketData::ChatMessage::ReadFrom(Packet& packet) {
    playerID = packet.ReadUint32();
    playerName = packet.ReadString();
    message = packet.ReadString();
}

void PacketData::EntityUpdate::WriteTo(Packet& packet) const {
    packet.WriteUint32(entityID);
    packet.WriteVec3(position);
    packet.WriteVec3(rotation);
    packet.WriteVec3(scale);
    packet.WriteUint8(isVisible ? 1 : 0);
}

void PacketData::EntityUpdate::ReadFrom(Packet& packet) {
    entityID = packet.ReadUint32();
    position = packet.ReadVec3();
    rotation = packet.ReadVec3();
    scale = packet.ReadVec3();
    isVisible = packet.ReadUint8() != 0;
}

void PacketData::PlayerJoin::WriteTo(Packet& packet) const {
    packet.WriteUint32(playerID);
    packet.WriteString(playerName);
    packet.WriteVec2(spawnPosition);
}

void PacketData::PlayerJoin::ReadFrom(Packet& packet) {
    playerID = packet.ReadUint32();
    playerName = packet.ReadString();
    spawnPosition = packet.ReadVec2();
}

void PacketData::PeerIDAssignment::WriteTo(Packet& packet) const {
    packet.WriteUint32(assignedPeerID);
}

void PacketData::PeerIDAssignment::ReadFrom(Packet& packet) {
    assignedPeerID = packet.ReadUint32();
}

// PacketFactory implementations

Packet PacketFactory::CreatePingPacket() {
    Packet packet(PacketType::PING);
    packet.WriteUint32(enet_time_get()); // Timestamp for latency calculation
    return packet;
}

Packet PacketFactory::CreatePongPacket() {
    Packet packet(PacketType::PONG);
    packet.WriteUint32(enet_time_get()); // Response timestamp
    return packet;
}

Packet PacketFactory::CreatePlayerMovePacket(const PacketData::PlayerMove& moveData) {
    Packet packet(PacketType::PLAYER_MOVE);
    moveData.WriteTo(packet);
    return packet;
}

Packet PacketFactory::CreateChatPacket(const PacketData::ChatMessage& chatData) {
    Packet packet(PacketType::CHAT_MESSAGE);
    chatData.WriteTo(packet);
    return packet;
}

Packet PacketFactory::CreateEntityUpdatePacket(const PacketData::EntityUpdate& entityData) {
    Packet packet(PacketType::ENTITY_UPDATE);
    entityData.WriteTo(packet);
    return packet;
}

Packet PacketFactory::CreatePlayerJoinPacket(const PacketData::PlayerJoin& joinData) {
    Packet packet(PacketType::PLAYER_JOIN);
    joinData.WriteTo(packet);
    return packet;
}

Packet PacketFactory::CreateDisconnectPacket(const std::string& reason) {
    Packet packet(PacketType::DISCONNECT);
    packet.WriteString(reason);
    return packet;
}

Packet PacketFactory::CreatePlayerLeavePacket(uint32_t playerID) {
    Packet packet(PacketType::PLAYER_LEAVE);
    packet.WriteUint32(playerID);
    return packet;
}

Packet PacketFactory::CreatePeerIDAssignmentPacket(uint32_t assignedPeerID) {
    Packet packet(PacketType::PEER_ID_ASSIGNMENT);
    packet.WriteUint32(assignedPeerID);
    return packet;
} 