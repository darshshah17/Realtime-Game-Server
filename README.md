# Multiplayer Game Server

A real-time multiplayer game server implementation in C++ with a C#/.NET client SDK. This project demonstrates server-side authoritative simulation with rollback/reconciliation, matchmaking, chat, and game state synchronization.

## Features

- **Real-time Multiplayer Support**: WebSocket-based communication supporting concurrent players
- **Server-side Authoritative Simulation**: Ensures fairness and anti-cheat protection
- **Rollback/Reconciliation**: Handles network latency and packet loss gracefully
- **Matchmaking System**: Automatic player matching based on game mode and player count
- **Chat System**: Global and channel-based chat messaging
- **Game State Synchronization**: Efficient state updates with tick-based simulation
- **C# Client SDK**: Easy-to-use SDK for game client integration

## Tech Stack

- **Server**: C++17
- **Client SDK**: C# / .NET 6.0
- **Communication**: WebSockets
- **Serialization**: JSON (using jsoncpp for C++, Newtonsoft.Json for C#)

## Project Structure

```
Game Server/
├── server/              # C++ server implementation
│   ├── main.cpp        # Server entry point
│   ├── GameServer.*    # Main server class
│   ├── WebSocketServer.*  # WebSocket server wrapper
│   ├── PlayerManager.*    # Player management
│   ├── MatchmakingSystem.* # Matchmaking logic
│   ├── ChatSystem.*       # Chat functionality
│   ├── GameStateManager.* # Game state and rollback/reconciliation
│   └── CMakeLists.txt     # Build configuration
├── sdk/                 # C# client SDK
│   ├── GameServerSDK.csproj
│   ├── GameServerClient.cs  # Core WebSocket client
│   ├── GameServerSDK.cs     # Main SDK entry point
│   ├── MatchmakingAPI.cs    # Matchmaking API
│   ├── ChatAPI.cs           # Chat API
│   ├── GameStateAPI.cs      # Game state API
│   └── EventArgs.cs         # Event argument classes
└── examples/            # Example client implementations
    └── ExampleClient/   # Basic C# client example
```

## Building the Server

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15+
- jsoncpp library
- WebSocket library (uWebSockets, libwebsockets, or similar)

### Build Steps

```bash
cd server
mkdir build
cd build
cmake ..
make
```

### Running the Server

```bash
./GameServer [port]
# Default port is 8080
```

## Building the SDK

### Prerequisites

- .NET 6.0 SDK or later

### Build Steps

```bash
cd sdk
dotnet build
```

### Using the SDK

Add a reference to the SDK in your project:

```xml
<ProjectReference Include="path/to/GameServerSDK.csproj" />
```

Or install as a NuGet package (if published).

## Usage Example

### C# Client

```csharp
using GameServerSDK;

var sdk = new GameServerSDK("ws://localhost:8080");

sdk.OnMatchFound += (sender, e) => {
    Console.WriteLine($"Match found: {e.MatchId}");
};

await sdk.ConnectAsync();
await sdk.Matchmaking.QueueForMatchAsync("default", 2, 4);
await sdk.Chat.SendMessageAsync("Hello!");
await sdk.GameState.SendMoveAsync(10.0, 20.0, 5.0);
```

## Demo & Testing

### Web-Based Visual Demo

The easiest way to demonstrate the server is using the included web client:

```bash
cd demo/web-client
python3 -m http.server 3000
# Open http://localhost:3000 in your browser
```

The web client provides a visual interface to:
- Connect to the server
- Request matchmaking
- Send chat messages
- Send game actions
- View real-time events and metrics

### Python Test Script

For automated testing and demonstrating concurrent connections:

```bash
cd demo/test-scripts
pip install -r requirements.txt
python3 test_server.py
```

This script demonstrates:
- Multiple concurrent WebSocket connections
- Matchmaking with multiple players
- Chat system functionality
- Game action handling
- Performance metrics

See `demo/DEMO_GUIDE.md` for comprehensive demo instructions.

## Architecture

### Server-Side Authoritative Simulation

The server maintains the authoritative game state and validates all player actions. This prevents cheating by ensuring:
- Actions are validated before being applied
- Game state is always consistent
- Client predictions can be corrected via reconciliation

### Rollback/Reconciliation

The server maintains snapshots of game state at regular intervals. When validation fails or corrections are needed:
1. Server rolls back to a previous snapshot
2. Replays actions from that point
3. Sends corrected state to clients
4. Clients reconcile their local state with server state

### Matchmaking

Players are queued by game mode and matched when enough players are available. The system:
- Groups players by game mode
- Matches players based on min/max player requirements
- Creates match instances and notifies all players

### Chat System

Supports multiple channels (global, match-specific, etc.):
- Message validation and moderation
- Message history per channel
- Real-time broadcasting

## Performance Considerations

- **Tick Rate**: 60 ticks per second (configurable)
- **Concurrent Players**: Designed to support 500+ concurrent players
- **Latency**: Target < 100ms per action (depends on network conditions)
- **State Updates**: Broadcasted efficiently to relevant players only

## WebSocket Library Integration

The current implementation includes a placeholder WebSocket server. For production use, integrate one of:

1. **uWebSockets** (Recommended): High performance, lightweight
2. **libwebsockets**: Mature, feature-rich
3. **Boost.Beast**: Header-only, part of Boost

Update `WebSocketServer.cpp` to use your chosen library.

## License

This project is provided as-is for educational and portfolio purposes.

## Notes

- The WebSocket server implementation is a placeholder and needs to be replaced with a production-ready library
- Additional game-specific logic should be added to `GameStateManager::simulateTick()` and `GameStateManager::applyAction()`
- Anti-cheat validation in `GameStateManager::validateAction()` should be expanded based on game requirements
- Consider adding authentication, rate limiting, and additional security measures for production use

