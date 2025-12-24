let ws = null;
let playerId = null;
let isConnected = false;
let pingInterval = null;
let lastPingTime = 0;

// Optimized State Management
let playerElements = {}; // Map<playerId, DOMElement>
let localPlayerPos = { x: 0, y: 0 }; // Local prediction state

// Colors for players (Grayscale/Monochrome)
const PLAYER_COLORS = [
    '#ffffff', '#dddddd', '#bbbbbb', '#999999', 
    '#777777', '#555555', '#e0e0e0', '#c0c0c0'
];

function log(message, type = 'info') {
    const logDiv = document.getElementById('log');
    const entry = document.createElement('div');
    entry.className = `log-entry ${type}`;
    entry.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
    logDiv.appendChild(entry);
    logDiv.scrollTop = logDiv.scrollHeight;
}

function updateStatus(connected) {
    const statusDiv = document.getElementById('status');
    const connectBtn = document.getElementById('connectBtn');
    const disconnectBtn = document.getElementById('disconnectBtn');
    
    isConnected = connected;
    
    if (connected) {
        statusDiv.textContent = 'Connected';
        statusDiv.className = 'status connected';
        connectBtn.disabled = true;
        disconnectBtn.disabled = false;
        document.getElementById('matchmakingBtn').disabled = false;
        document.getElementById('chatBtn').disabled = false;
        document.getElementById('joinGameBtn').disabled = false;
    } else {
        statusDiv.textContent = 'Disconnected';
        statusDiv.className = 'status disconnected';
        connectBtn.disabled = false;
        disconnectBtn.disabled = true;
        document.getElementById('matchmakingBtn').disabled = true;
        document.getElementById('chatBtn').disabled = true;
        document.getElementById('joinGameBtn').disabled = true;
        document.getElementById('cancelMatchmakingBtn').disabled = true;
    }
}

function connect() {
    const url = document.getElementById('serverUrl').value;
    if (!url) return;
    
    if (ws) {
        ws.close();
        ws = null;
        setTimeout(() => createWebSocketConnection(url), 100);
        return;
    }
    createWebSocketConnection(url);
}

function createWebSocketConnection(url) {
    try {
        if (url.startsWith('wss://')) url = url.replace('wss://', 'ws://');
        
        // log('Attempting to connect to: ' + url, 'info'); // Removed
        ws = new WebSocket(url);
        
        ws.onopen = () => {
            // log('Connected to server', 'success'); // Removed, waiting for ID
            updateStatus(true);
            pingInterval = setInterval(() => {
                if (ws && ws.readyState === WebSocket.OPEN) {
                    lastPingTime = Date.now();
                    sendMessage({ type: 'ping' });
                }
            }, 5000);
            initGrid();
        };
        
        ws.onclose = (event) => {
            log('Disconnected', 'warning');
            updateStatus(false);
            if (pingInterval) clearInterval(pingInterval);
            resetGame();
        };
        
        ws.onerror = (error) => {
            log('WebSocket error', 'error');
        };
        
        ws.onmessage = (event) => {
            try {
                handleMessage(JSON.parse(event.data));
            } catch (e) { }
        };
    } catch (e) {
        log('Connection error', 'error');
    }
}

function disconnect() {
    if (ws) ws.close();
    updateStatus(false);
    resetGame();
}

function sendMessage(message) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify(message));
    }
}

function handleMessage(message) {
    switch (message.type) {
        case 'connected':
            playerId = message.playerId;
            document.getElementById('playerId').textContent = playerId || '-';
            log(`Connected to server as Player ${playerId}`, 'success');
            break;
        case 'match_found':
            log(`Match Found!`, 'match');
            break;
        case 'chat_message':
            log(`[${message.channel}] ${message.username || 'Unknown'}: ${message.message}`, 'chat');
            break;
        case 'state_update':
            document.getElementById('serverTime').textContent = message.serverTime || '-';
            handleStateUpdate(message);
            break;
        case 'pong':
            document.getElementById('latency').textContent = Date.now() - lastPingTime;
            break;
    }
}

function requestMatchmaking() {
    const gameMode = document.getElementById('gameMode').value;
    const min = parseInt(document.getElementById('minPlayers').value);
    const max = parseInt(document.getElementById('maxPlayers').value);
    sendMessage({ type: 'matchmaking_request', gameMode, minPlayers: min, maxPlayers: max });
    
    // UI Update
    document.getElementById('matchmakingBtn').disabled = true;
    document.getElementById('cancelMatchmakingBtn').disabled = false;
    log(`Requesting match...`, 'info');
}

function cancelMatchmaking() { 
    sendMessage({ type: 'cancel_matchmaking' }); // Placeholder
    // UI Update
    document.getElementById('matchmakingBtn').disabled = false;
    document.getElementById('cancelMatchmakingBtn').disabled = true;
    log('Matchmaking cancelled', 'warning');
}

function sendChat() {
    const msg = document.getElementById('chatMessage').value;
    if (msg) {
        sendMessage({ type: 'chat_message', message: msg, channel: 'global' });
        document.getElementById('chatMessage').value = '';
    }
}

// --- OPTIMIZED GAME LOGIC ---

let isGameFocused = false;
let globalListenerAdded = false;
let lastKeyTime = 0; // Debounce for repeat

function initGrid() {
    const grid = document.getElementById('gameGrid');
    if (grid.children.length > 0) return; // Don't rebuild if exists
    
    grid.innerHTML = '';
    for (let y = 0; y < 8; y++) {
        for (let x = 0; x < 8; x++) {
            const cell = document.createElement('div');
            cell.className = 'grid-cell';
            cell.id = `cell-${x}-${y}`;
            cell.onclick = () => {
                isGameFocused = true;
                grid.style.borderColor = '#ffffff';
            };
            grid.appendChild(cell);
        }
    }
    
    if (!globalListenerAdded) {
        window.addEventListener('keydown', handleKeyPress);
        globalListenerAdded = true;
        document.addEventListener('click', (e) => {
            if (!e.target.closest('#gameGrid') && !e.target.closest('#joinGameBtn')) {
                isGameFocused = false;
                document.getElementById('gameGrid').style.borderColor = '#333';
            }
        });
    }
}

function resetGame() {
    playerElements = {};
    const cells = document.querySelectorAll('.grid-cell');
    cells.forEach(c => c.innerHTML = '');
}

function joinGame() {
    sendMessage({
        type: 'game_action',
        actionType: 'spawn',
        actionId: Date.now(),
        timestamp: Date.now()
    });
    log('Joining game...', 'info');
    isGameFocused = true;
    document.getElementById('gameGrid').style.borderColor = '#ffffff';
}

function handleKeyPress(e) {
    if (!isConnected || !isGameFocused) return;
    if(['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', ' '].indexOf(e.code) > -1) e.preventDefault();
    
    // Prevent key repeat spamming moves too fast (50ms limit)
    const now = Date.now();
    if (now - lastKeyTime < 50) return;
    lastKeyTime = now;

    let action = null;
    let data = {};
    let dx = 0, dy = 0;
    
    if (e.key === 'ArrowUp') { dx = 0; dy = -1; }
    else if (e.key === 'ArrowDown') { dx = 0; dy = 1; }
    else if (e.key === 'ArrowLeft') { dx = -1; dy = 0; }
    else if (e.key === 'ArrowRight') { dx = 1; dy = 0; }
    else return;

    action = e.shiftKey ? 'shoot' : 'move';
    data = { dx, dy };

    if (action === 'move') {
        const currentX = localPlayerPos.x;
        const currentY = localPlayerPos.y;
        const newX = currentX + dx;
        const newY = currentY + dy;

        if (newX >= 0 && newX <= 7 && newY >= 0 && newY <= 7) {
            localPlayerPos = { x: newX, y: newY };
            updatePlayerToken(playerId, newX, newY, true);
        }
    }

    sendMessage({
        type: 'game_action',
        actionType: action,
        actionId: now,
        timestamp: now,
        data: data
    });
}

function handleStateUpdate(message) {
    if (!message.state || !message.state.players) return;
    const players = message.state.players;
    
    for (let id in playerElements) {
        if (!players[id]) {
            if (playerElements[id]) playerElements[id].remove();
            delete playerElements[id];
        }
    }

    for (let id in players) {
        const p = players[id];
        const isMe = (parseInt(id) === playerId);
        
        // Only update local position from server if we are NOT moving locally,
        // or if we are totally desynced (reconciliation).
        // For simplicity, we trust local move prediction for rendering 
        // unless it's been a while or we just spawned.
        if (isMe) {
            // Check drift distance
            const dist = Math.abs(p.x - localPlayerPos.x) + Math.abs(p.y - localPlayerPos.y);
            // If server disagrees by >1 square, snap back (anti-cheat or lag correction)
            if (dist > 1) {
                localPlayerPos = { x: p.x, y: p.y };
                updatePlayerToken(id, p.x, p.y, isMe);
            }
            // Else: Trust client prediction, ignore server packet for MY rendering
        } else {
            // Other players: Always trust server
            updatePlayerToken(id, p.x, p.y, isMe);
        }
    }
}

function updatePlayerToken(id, x, y, isMe) {
    if (x < 0 || x > 7 || y < 0 || y > 7) return;

    let el = playerElements[id];
    
    if (!el) {
        el = document.createElement('div');
        el.className = 'player-token';
        
        const colorIdx = (parseInt(id) % PLAYER_COLORS.length);
        const bgColor = PLAYER_COLORS[colorIdx];
        el.style.backgroundColor = bgColor;
        
        // Adjust text color for contrast
        if (bgColor === '#555555' || bgColor === '#777777') {
             el.style.color = '#fff';
        } else {
             el.style.color = '#000';
        }
        
        if (isMe) {
            el.style.border = '2px solid #000'; // High contrast against white token
            el.style.outline = '2px solid #fff'; // Double border effect
            el.textContent = '★';
        } else {
            el.classList.add('other-player');
            el.textContent = id;
        }
        
        playerElements[id] = el;
    }
    
    const targetCell = document.getElementById(`cell-${x}-${y}`);
    if (targetCell && el.parentElement !== targetCell) {
        targetCell.appendChild(el);
    }
}

initGrid();
