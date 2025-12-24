using System;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace GameServerSDK
{
    /// <summary>
    /// Main client class for connecting to the game server
    /// </summary>
    public class GameServerClient : IDisposable
    {
        private ClientWebSocket? _webSocket;
        private CancellationTokenSource? _cancellationTokenSource;
        private Task? _receiveTask;
        private readonly string _serverUrl;
        private bool _isConnected;
        private ulong _playerId;
        private ulong _sequenceNumber;

        // Events
        public event EventHandler<ConnectedEventArgs>? OnConnected;
        public event EventHandler<DisconnectedEventArgs>? OnDisconnected;
        public event EventHandler<MatchFoundEventArgs>? OnMatchFound;
        public event EventHandler<ChatMessageEventArgs>? OnChatMessage;
        public event EventHandler<StateUpdateEventArgs>? OnStateUpdate;
        public event EventHandler<ErrorEventArgs>? OnError;

        public bool IsConnected => _isConnected && _webSocket?.State == WebSocketState.Open;
        public ulong PlayerId => _playerId;

        public GameServerClient(string serverUrl = "ws://localhost:8080")
        {
            _serverUrl = serverUrl;
            _sequenceNumber = 0;
        }

        /// <summary>
        /// Connects to the game server
        /// </summary>
        public async Task ConnectAsync(CancellationToken cancellationToken = default)
        {
            if (_isConnected)
            {
                throw new InvalidOperationException("Already connected to server");
            }

            _webSocket = new ClientWebSocket();
            _cancellationTokenSource = new CancellationTokenSource();

            try
            {
                await _webSocket.ConnectAsync(new Uri(_serverUrl), cancellationToken);
                _isConnected = true;

                _receiveTask = Task.Run(() => ReceiveLoopAsync(_cancellationTokenSource.Token));

                OnConnected?.Invoke(this, new ConnectedEventArgs { PlayerId = _playerId });
            }
            catch (Exception ex)
            {
                _isConnected = false;
                OnError?.Invoke(this, new ErrorEventArgs { Exception = ex, Message = "Failed to connect to server" });
                throw;
            }
        }

        /// <summary>
        /// Disconnects from the game server
        /// </summary>
        public async Task DisconnectAsync()
        {
            if (!_isConnected)
            {
                return;
            }

            _isConnected = false;
            _cancellationTokenSource?.Cancel();

            if (_webSocket != null && _webSocket.State == WebSocketState.Open)
            {
                await _webSocket.CloseAsync(WebSocketCloseStatus.NormalClosure, "Client disconnecting", CancellationToken.None);
            }

            if (_receiveTask != null)
            {
                await _receiveTask;
            }

            _webSocket?.Dispose();
            _webSocket = null;

            OnDisconnected?.Invoke(this, new DisconnectedEventArgs());
        }

        private async Task ReceiveLoopAsync(CancellationToken cancellationToken)
        {
            var buffer = new byte[4096];

            while (!cancellationToken.IsCancellationRequested && _webSocket?.State == WebSocketState.Open)
            {
                try
                {
                    var result = await _webSocket.ReceiveAsync(new ArraySegment<byte>(buffer), cancellationToken);

                    if (result.MessageType == WebSocketMessageType.Close)
                    {
                        await _webSocket.CloseAsync(WebSocketCloseStatus.NormalClosure, "Server closed connection", CancellationToken.None);
                        break;
                    }

                    if (result.MessageType == WebSocketMessageType.Text)
                    {
                        var message = Encoding.UTF8.GetString(buffer, 0, result.Count);
                        HandleMessage(message);
                    }
                }
                catch (OperationCanceledException)
                {
                    break;
                }
                catch (Exception ex)
                {
                    OnError?.Invoke(this, new ErrorEventArgs { Exception = ex, Message = "Error receiving message" });
                    break;
                }
            }

            _isConnected = false;
        }

        private void HandleMessage(string message)
        {
            try
            {
                var json = JObject.Parse(message);
                var type = json["type"]?.ToString();

                switch (type)
                {
                    case "connected":
                        _playerId = json["playerId"]?.ToObject<ulong>() ?? 0;
                        OnConnected?.Invoke(this, new ConnectedEventArgs { PlayerId = _playerId });
                        break;

                    case "match_found":
                        var matchFound = json.ToObject<MatchFoundEventArgs>();
                        OnMatchFound?.Invoke(this, matchFound ?? new MatchFoundEventArgs());
                        break;

                    case "chat_message":
                        var chatMsg = json.ToObject<ChatMessageEventArgs>();
                        OnChatMessage?.Invoke(this, chatMsg ?? new ChatMessageEventArgs());
                        break;

                    case "state_update":
                        var stateUpdate = json.ToObject<StateUpdateEventArgs>();
                        OnStateUpdate?.Invoke(this, stateUpdate ?? new StateUpdateEventArgs());
                        break;

                    case "pong":
                        // Handle ping response
                        break;

                    default:
                        Console.WriteLine($"Unknown message type: {type}");
                        break;
                }
            }
            catch (Exception ex)
            {
                OnError?.Invoke(this, new ErrorEventArgs { Exception = ex, Message = "Error parsing message" });
            }
        }

        private async Task SendMessageAsync(object message)
        {
            if (!IsConnected || _webSocket == null)
            {
                throw new InvalidOperationException("Not connected to server");
            }

            var json = JsonConvert.SerializeObject(message);
            var bytes = Encoding.UTF8.GetBytes(json);
            await _webSocket.SendAsync(new ArraySegment<byte>(bytes), WebSocketMessageType.Text, true, CancellationToken.None);
        }

        /// <summary>
        /// Requests to join matchmaking queue
        /// </summary>
        public async Task RequestMatchmakingAsync(string gameMode = "default", int minPlayers = 2, int maxPlayers = 4)
        {
            var request = new
            {
                type = "matchmaking_request",
                gameMode = gameMode,
                minPlayers = minPlayers,
                maxPlayers = maxPlayers
            };

            await SendMessageAsync(request);
        }

        /// <summary>
        /// Sends a chat message
        /// </summary>
        public async Task SendChatMessageAsync(string message, string channel = "global")
        {
            var chatRequest = new
            {
                type = "chat_message",
                message = message,
                channel = channel
            };

            await SendMessageAsync(chatRequest);
        }

        /// <summary>
        /// Sends a game action to the server
        /// </summary>
        public async Task SendGameActionAsync(string actionType, object actionData)
        {
            var action = new
            {
                type = "game_action",
                actionId = ++_sequenceNumber,
                timestamp = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds(),
                actionType = actionType,
                data = actionData,
                sequenceNumber = _sequenceNumber
            };

            await SendMessageAsync(action);
        }

        /// <summary>
        /// Sends a ping to measure latency
        /// </summary>
        public async Task PingAsync()
        {
            var ping = new
            {
                type = "ping"
            };

            await SendMessageAsync(ping);
        }

        public void Dispose()
        {
            DisconnectAsync().Wait(TimeSpan.FromSeconds(5));
            _cancellationTokenSource?.Dispose();
        }
    }
}

