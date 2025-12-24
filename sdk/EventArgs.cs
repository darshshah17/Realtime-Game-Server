using System;

namespace GameServerSDK
{
    public class ConnectedEventArgs : EventArgs
    {
        public ulong PlayerId { get; set; }
        public ulong ServerTime { get; set; }
    }

    public class DisconnectedEventArgs : EventArgs
    {
        public string? Reason { get; set; }
    }

    public class MatchFoundEventArgs : EventArgs
    {
        public string? MatchId { get; set; }
        public string? GameMode { get; set; }
        public ulong[]? Players { get; set; }
    }

    public class ChatMessageEventArgs : EventArgs
    {
        public ulong PlayerId { get; set; }
        public string? Username { get; set; }
        public string? Message { get; set; }
        public ulong Timestamp { get; set; }
        public string? Channel { get; set; }
    }

    public class StateUpdateEventArgs : EventArgs
    {
        public ulong ServerTime { get; set; }
        public ulong Tick { get; set; }
        public object? State { get; set; }
    }

    public class ErrorEventArgs : EventArgs
    {
        public Exception? Exception { get; set; }
        public string? Message { get; set; }
    }
}

