# GLFWEng Networking System

A comprehensive networking system built on top of ENet for the GLFWEng game engine. This system provides client-server networking with packet serialization, event handling, and easy integration.

## Features

- **Client-Server Architecture**: Support for both server and client modes
- **Packet Serialization**: Type-safe packet creation and parsing with built-in data types
- **Event-Driven**: Callback-based event handling for network events
- **Reliability Options**: Support for reliable, unreliable, and unsequenced packet delivery
- **Built-in Systems**: Automatic ping/pong, connection management, and statistics
- **Easy Integration**: Simple API that integrates with the existing engine lifecycle

## Quick Start

### 1. Basic Setup

Include the networking headers and initialize the system:

```cpp
#include <engine/core/networking/NetworkManager.h>
#include <engine/core/networking/Packet.h>

// In your Game::OnInit()
if (!Network::Initialize()) {
    Logger::Error("Failed to initialize networking");
    return;
}

// Set up event handling
Network::GetManager().SetEventCallback([this](const NetworkEvent& event) {
    HandleNetworkEvent(event);
});
```

### 2. Starting a Server

```cpp
// Start server on port 7777 with max 10 clients
if (Network::StartServer(7777, 10)) {
    Logger::Info("Server started successfully");
} else {
    Logger::Error("Failed to start server: " + NetworkManager::GetLastError());
}
```

### 3. Connecting as Client

```cpp
// Connect to localhost:7777
if (Network::ConnectToServer("localhost", 7777)) {
    Logger::Info("Connected to server");
} else {
    Logger::Error("Failed to connect: " + NetworkManager::GetLastError());
}
```

### 4. Sending Packets

```cpp
// Create a chat message packet
PacketData::ChatMessage chatData;
chatData.playerID = 1;
chatData.playerName = "Player1";
chatData.message = "Hello, world!";

Packet chatPacket = PacketFactory::CreateChatPacket(chatData);

// Send to server (as client) or specific peer (as server)
Network::SendPacket(chatPacket, peerID);

// Broadcast to all clients (server only)
Network::BroadcastPacket(chatPacket);
```

### 5. Update Loop

```cpp
// In your Game::OnUpdate()
Network::Update();
```

### 6. Cleanup

```cpp
// In your Game::OnShutdown()
Network::Shutdown();
```

## Packet System

### Built-in Packet Types

- `PING`/`PONG` - Automatic latency measurement
- `PLAYER_MOVE` - Player movement updates
- `PLAYER_JOIN`/`PLAYER_LEAVE` - Player connection events
- `CHAT_MESSAGE` - Text chat
- `ENTITY_UPDATE` - Generic entity updates
- `CUSTOM_GAME_EVENT` - Your custom packets (starting from 100)

### Creating Custom Packets

```cpp
// 1. Define your packet type (add to PacketType enum)
enum class PacketType : uint8_t {
    // ... existing types ...
    MY_CUSTOM_PACKET = 101
};

// 2. Create packet data structure
struct MyCustomData {
    uint32_t someID;
    std::string someString;
    glm::vec3 someVector;
    
    void WriteTo(Packet& packet) const {
        packet.WriteUint32(someID);
        packet.WriteString(someString);
        packet.WriteVec3(someVector);
    }
    
    void ReadFrom(Packet& packet) {
        someID = packet.ReadUint32();
        someString = packet.ReadString();
        someVector = packet.ReadVec3();
    }
};

// 3. Create factory method
Packet CreateMyCustomPacket(const MyCustomData& data) {
    Packet packet(PacketType::MY_CUSTOM_PACKET);
    data.WriteTo(packet);
    return packet;
}

// 4. Register packet handler
Network::GetManager().RegisterPacketHandler(PacketType::MY_CUSTOM_PACKET,
    [this](const Packet& packet, uint32_t senderID) {
        MyCustomData data;
        Packet mutablePacket = packet;
        data.ReadFrom(mutablePacket);
        
        // Handle your custom packet data
        HandleMyCustomPacket(data, senderID);
    });
```

### Packet Reliability

Choose the appropriate reliability for your packets:

```cpp
// Reliable: Guaranteed delivery, ordered (use for important data)
Network::SendPacket(packet, peerID, PacketReliability::RELIABLE);

// Unreliable: Fast, no guarantees (use for frequent updates like position)
Network::SendPacket(packet, peerID, PacketReliability::UNRELIABLE);

// Unsequenced: Guaranteed but unordered (use for independent events)
Network::SendPacket(packet, peerID, PacketReliability::UNSEQUENCED);
```

## Event Handling

Handle network events through the callback system:

```cpp
void HandleNetworkEvent(const NetworkEvent& event) {
    switch (event.type) {
        case NetworkEventType::CLIENT_CONNECTED:
            Logger::Info("Client connected: " + std::to_string(event.peerID));
            // Send welcome packet, add to player list, etc.
            break;
            
        case NetworkEventType::CLIENT_DISCONNECTED:
            Logger::Info("Client disconnected: " + std::to_string(event.peerID));
            // Remove from player list, cleanup, etc.
            break;
            
        case NetworkEventType::SERVER_CONNECTED:
            Logger::Info("Connected to server");
            // Send join packet, request game state, etc.
            break;
            
        case NetworkEventType::PACKET_RECEIVED:
            // Handle packets not caught by registered handlers
            break;
            
        // ... handle other events
    }
}
```

## Configuration

### Bandwidth Limiting

```cpp
// Limit to 1MB/s incoming, 512KB/s outgoing
Network::GetManager().SetBandwidthLimit(1024 * 1024, 512 * 1024);
```

### Compression

```cpp
// Enable packet compression (reduces bandwidth but increases CPU usage)
Network::GetManager().SetCompressionEnabled(true);
```

### Channel Limits

```cpp
// Set number of packet channels (default: 2)
Network::GetManager().SetChannelLimit(4);
```

## Statistics and Monitoring

```cpp
auto& manager = Network::GetManager();

// Get connection statistics
uint32_t packetsSent = manager.GetPacketsSent();
uint32_t packetsReceived = manager.GetPacketsReceived();
uint64_t bytesSent = manager.GetBytesSent();
uint64_t bytesReceived = manager.GetBytesReceived();

// Get latency to server (client) or specific peer (server)
uint32_t latency = manager.GetLatency(peerID);

// Get connected peers info
for (const auto& peer : manager.GetConnectedPeers()) {
    Logger::Info("Peer " + std::to_string(peer.id) + 
                " at " + peer.address + ":" + std::to_string(peer.port) +
                " (RTT: " + std::to_string(peer.roundTripTime) + "ms)");
}
```

## Error Handling

Always check return values and handle errors:

```cpp
if (!Network::StartServer(7777)) {
    std::string error = NetworkManager::GetLastError();
    Logger::Error("Server start failed: " + error);
    // Handle error appropriately
}

if (!Network::SendPacket(packet, peerID)) {
    std::string error = NetworkManager::GetLastError();
    Logger::Error("Packet send failed: " + error);
    // Maybe queue for retry, or inform user
}
```

## Example Integration

See `NetworkExample.h` for a complete example showing:
- Server/client initialization
- Packet handling
- Event management
- Input handling for network commands
- Integration with game loop

### Key Bindings in Example

- **F2**: Start server
- **F3**: Connect to server as client  
- **F4**: Send test chat message

## Best Practices

1. **Packet Frequency**: Don't send packets every frame for non-critical data. Use timers to limit update rates.

2. **Reliability Choice**: Use unreliable packets for frequent updates (position, rotation) and reliable packets for important events (player joined, item picked up).

3. **Error Handling**: Always check return values and handle network errors gracefully.

4. **State Synchronization**: Implement client prediction and server reconciliation for smooth gameplay.

5. **Security**: Validate all incoming packet data. Never trust client input for authoritative game state.

6. **Bandwidth**: Monitor network usage and implement compression or data reduction techniques for large packets.

## Integration with ECS

The networking system can easily integrate with your ECS by:

1. Creating network components for entities that need synchronization
2. Implementing systems that handle network updates
3. Using entity IDs in packets to sync specific entities
4. Creating networked entity spawning/destruction systems

```cpp
// Example: Network sync component
struct NetworkSyncComponent {
    uint32_t networkID;
    bool isDirty;
    float lastSyncTime;
};

// Example: Network sync system
class NetworkSyncSystem : public System {
    void Update(Scene* scene, float deltaTime) override {
        auto entities = scene->GetEntitiesWith<TransformComponent, NetworkSyncComponent>();
        
        for (auto& entity : entities) {
            auto* transform = entity.GetComponent<TransformComponent>();
            auto* netSync = entity.GetComponent<NetworkSyncComponent>();
            
            if (netSync->isDirty && IsServer()) {
                // Send entity update to clients
                PacketData::EntityUpdate updateData;
                updateData.entityID = netSync->networkID;
                updateData.position = transform->position;
                // ... fill other data
                
                Packet packet = PacketFactory::CreateEntityUpdatePacket(updateData);
                Network::BroadcastPacket(packet, PacketReliability::UNRELIABLE);
                
                netSync->isDirty = false;
                netSync->lastSyncTime = Time::GetTime();
            }
        }
    }
};
```

## Troubleshooting

### Common Issues

1. **Connection timeouts**: Check firewall settings and ensure ports are open
2. **Packet loss**: Use reliable packets for critical data
3. **High latency**: Reduce packet frequency and payload size
4. **Memory leaks**: Ensure all ENet packets are properly destroyed (handled automatically by the system)

### Debug Logging

Enable detailed logging by setting log levels in your Logger configuration to see detailed network events and packet information. 