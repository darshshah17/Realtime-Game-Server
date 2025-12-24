# Realtime Game Server

A high-performance real-time multiplayer game server implementation in C++ and C# with WebSocket support. Features server-side authoritative simulation, matchmaking, chat, and an interactive 8x8 grid-based game.

<img width="1470" height="828" alt="Screenshot 2025-12-23 at 10 34 34 PM" src="https://github.com/user-attachments/assets/39aca4d1-a032-43e8-8563-a2d1b5cd170e" />


## Features

1) **High-Performance Server**: C++ server running at 120 ticks/second with ultra-low latency network polling

2) **Real-time Multiplayer**: WebSocket-based communication supporting multiple concurrent players
  
4) **Server-side Authoritative**: Ensures game integrity and prevents cheating
  
5) **Matchmaking System**: Automatic player matching based on game mode and player count
  
6) **Chat System**: Global and channel-based chat messaging
  
7) **Client-side Prediction**: Instant local feedback for smooth player experience
  
8) **Web Client**: Web interface for testing and playing
  
9) **C# Client SDK**: Easy-to-use SDK for game client integration
   
<img width="800" height="650" alt="Screenshot 2025-12-24 at 2 55 54 PM" src="https://github.com/user-attachments/assets/d1bea60b-bfd3-417e-bf12-4413a587e389" />
<img width="400" height="228" alt="Screenshot 2025-12-24 at 2 30 16 PM" src="https://github.com/user-attachments/assets/8348bb38-16e3-4d15-a1c2-33938f7d8be3" />
<img width="400" height="228" alt="Screenshot 2025-12-24 at 2 31 18 PM" src="https://github.com/user-attachments/assets/98210ad2-e78f-4e1e-8ac6-b9ce9dcd9a68" />


## Building the Server

### Prerequisites

- libwebsockets
- jsoncpp

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
cd server/build
./GameServer 8080
```

## Building the SDK

### Prerequisites

- .NET 6.0 SDK or later

### Build Steps

```bash
cd sdk
dotnet build
```

## Architecture

### Server-Side Authoritative Simulation

The server maintains the authoritative game state and validates all player actions. This prevents cheating by ensuring:
- Actions are validated before being applied
- Game state is always consistent across all clients
- State updates are broadcast only when changes occur (dirty state tracking)

### Client-Side Prediction

The web client implements client-side prediction for instant local feedback:
- Players see their own moves immediately (0ms latency)
- Server confirms and corrects if needed
- Smooth gameplay experience even with network latency

### Matchmaking

Players are queued by game mode and matched when enough players are available. The system:
- Groups players by game mode
- Matches players based on min/max player requirements
- Creates match instances and notifies all players

### Chat System

Supports multiple channels (global, match-specific, etc.):
- Real-time message broadcasting
- Activity log integration

## Performance

- **Tick Rate**: 120 ticks per second for ultra-low latency
- **Network Polling**: 1ms timeout for minimal delay
- **State Updates**: Only broadcasted when game state changes (dirty tracking)
- **Client Prediction**: Instant local feedback with server reconciliation
- **Optimized Rendering**: DOM recycling and efficient updates in web client
