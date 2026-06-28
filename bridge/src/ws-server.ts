import { WebSocketServer, WebSocket } from 'ws';

export class WsServer {
  private wss: WebSocketServer;
  private clients = new Set<WebSocket>();

  constructor(port: number = 9120) {
    this.wss = new WebSocketServer({ port, host: '0.0.0.0' });
    this.wss.on('connection', (ws) => {
      console.log('[ws] client connected');
      this.clients.add(ws);
      ws.on('close', () => {
        console.log('[ws] client disconnected');
        this.clients.delete(ws);
      });
      ws.on('error', (err) => {
        console.error('[ws] error:', err.message);
        this.clients.delete(ws);
      });
    });
    console.log(`[ws] listening on ws://0.0.0.0:${port}`);
  }

  broadcast(state: string): void {
    const msg = JSON.stringify({ state });
    for (const ws of this.clients) {
      try {
        if (ws.readyState === WebSocket.OPEN) {
          ws.send(msg);
        }
      } catch {
        this.clients.delete(ws);
      }
    }
  }

  close(): void {
    this.wss.close();
  }
}
