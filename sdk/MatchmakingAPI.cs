using System;
using System.Threading.Tasks;

namespace GameServerSDK
{
    /// <summary>
    /// High-level API for matchmaking operations
    /// </summary>
    public class MatchmakingAPI
    {
        private readonly GameServerClient _client;

        public MatchmakingAPI(GameServerClient client)
        {
            _client = client;
        }

        /// <summary>
        /// Queues the player for matchmaking with default settings
        /// </summary>
        public async Task QueueForMatchAsync()
        {
            await _client.RequestMatchmakingAsync();
        }

        /// <summary>
        /// Queues the player for a specific game mode
        /// </summary>
        public async Task QueueForMatchAsync(string gameMode, int minPlayers = 2, int maxPlayers = 4)
        {
            await _client.RequestMatchmakingAsync(gameMode, minPlayers, maxPlayers);
        }

        /// <summary>
        /// Cancels matchmaking (disconnects from server)
        /// </summary>
        public async Task CancelMatchmakingAsync()
        {
            await _client.DisconnectAsync();
        }
    }
}

