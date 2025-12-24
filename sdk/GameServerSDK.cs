using System;
using System.Threading.Tasks;

namespace GameServerSDK
{
    /// <summary>
    /// Main SDK entry point providing easy access to all APIs
    /// </summary>
    public class GameServerSDK
    {
        private readonly GameServerClient _client;
        public MatchmakingAPI Matchmaking { get; }
        public ChatAPI Chat { get; }
        public GameStateAPI GameState { get; }

        public GameServerClient Client => _client;

        public GameServerSDK(string serverUrl = "ws://localhost:8080")
        {
            _client = new GameServerClient(serverUrl);
            Matchmaking = new MatchmakingAPI(_client);
            Chat = new ChatAPI(_client);
            GameState = new GameStateAPI(_client);
        }

        /// <summary>
        /// Connects to the game server
        /// </summary>
        public async Task ConnectAsync()
        {
            await _client.ConnectAsync();
        }

        /// <summary>
        /// Disconnects from the game server
        /// </summary>
        public async Task DisconnectAsync()
        {
            await _client.DisconnectAsync();
        }

        /// <summary>
        /// Checks if connected to the server
        /// </summary>
        public bool IsConnected => _client.IsConnected;

        /// <summary>
        /// Gets the player ID assigned by the server
        /// </summary>
        public ulong PlayerId => _client.PlayerId;

        // Expose events from client
        public event EventHandler<ConnectedEventArgs>? OnConnected
        {
            add => _client.OnConnected += value;
            remove => _client.OnConnected -= value;
        }

        public event EventHandler<DisconnectedEventArgs>? OnDisconnected
        {
            add => _client.OnDisconnected += value;
            remove => _client.OnDisconnected -= value;
        }

        public event EventHandler<MatchFoundEventArgs>? OnMatchFound
        {
            add => _client.OnMatchFound += value;
            remove => _client.OnMatchFound -= value;
        }

        public event EventHandler<ChatMessageEventArgs>? OnChatMessage
        {
            add => _client.OnChatMessage += value;
            remove => _client.OnChatMessage -= value;
        }

        public event EventHandler<StateUpdateEventArgs>? OnStateUpdate
        {
            add => _client.OnStateUpdate += value;
            remove => _client.OnStateUpdate -= value;
        }

        public event EventHandler<ErrorEventArgs>? OnError
        {
            add => _client.OnError += value;
            remove => _client.OnError -= value;
        }
    }
}

