using System;
using System.Threading.Tasks;
using GameServerSDK;

namespace ExampleClient
{
    class Program
    {
        static async Task Main(string[] args)
        {
            // Create SDK instance
            var sdk = new GameServerSDK("ws://localhost:8080");

            // Set up event handlers
            sdk.OnConnected += (sender, e) =>
            {
                Console.WriteLine($"Connected! Player ID: {e.PlayerId}");
            };

            sdk.OnDisconnected += (sender, e) =>
            {
                Console.WriteLine("Disconnected from server");
            };

            sdk.OnMatchFound += (sender, e) =>
            {
                Console.WriteLine($"Match found! Match ID: {e.MatchId}, Game Mode: {e.GameMode}");
                if (e.Players != null)
                {
                    Console.WriteLine($"Players: {string.Join(", ", e.Players)}");
                }
            };

            sdk.OnChatMessage += (sender, e) =>
            {
                Console.WriteLine($"[{e.Channel}] {e.Username}: {e.Message}");
            };

            sdk.OnStateUpdate += (sender, e) =>
            {
                // Handle game state updates
                // Console.WriteLine($"State update at tick {e.Tick}");
            };

            sdk.OnError += (sender, e) =>
            {
                Console.WriteLine($"Error: {e.Message}");
                if (e.Exception != null)
                {
                    Console.WriteLine($"Exception: {e.Exception.Message}");
                }
            };

            try
            {
                // Connect to server
                await sdk.ConnectAsync();
                Console.WriteLine("Connected to server. Waiting for connection confirmation...");
                await Task.Delay(1000);

                // Example: Request matchmaking
                Console.WriteLine("Requesting matchmaking...");
                await sdk.Matchmaking.QueueForMatchAsync("default", 2, 4);

                // Example: Send chat message
                await Task.Delay(2000);
                Console.WriteLine("Sending chat message...");
                await sdk.Chat.SendMessageAsync("Hello from C# client!");

                // Example: Send game actions
                await Task.Delay(2000);
                Console.WriteLine("Sending move action...");
                await sdk.GameState.SendMoveAsync(10.0, 20.0, 5.0);

                // Keep connection alive
                Console.WriteLine("Press any key to disconnect...");
                Console.ReadKey();

                // Disconnect
                await sdk.DisconnectAsync();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Fatal error: {ex.Message}");
            }
        }
    }
}

