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
      ws.on('error', (err: Error) => {
        console.error('[ws] error:', err.message);
        this.clients.delete(ws);
      });
    });
    console.log(`[ws] listening on ws://0.0.0.0:${port}`);
  }

  broadcastRaw(msg: string): void {
    for (const ws of this.clients) {
      try {
        if (ws.readyState === WebSocket.OPEN) ws.send(msg);
      } catch { this.clients.delete(ws); }
    }
  }

  broadcast(state: string): void {
    const msg = JSON.stringify({ state });
    console.log(`[ws] broadcast "${state}" to ${this.clients.size} clients`);
    for (const ws of this.clients) {
      try {
        if (ws.readyState === WebSocket.OPEN) {
          ws.send(msg, (err?: Error) => {
            if (err) console.error(`[ws] send error: ${err.message}`);
          });
        } else {
          console.log(`[ws] client not OPEN (state=${ws.readyState})`);
        }
      } catch (err: any) {
        console.error(`[ws] send exception: ${err?.message}`);
        this.clients.delete(ws);
      }
    }
  }

  close(): void {
    this.wss.close();
  }
}
