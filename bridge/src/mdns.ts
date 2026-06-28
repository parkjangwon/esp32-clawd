import { lookup } from 'node:dns/promises';

export async function discoverEsp32(host: string): Promise<string | null> {
  try {
    const mdn = `${host}.local`;
    const { address } = await lookup(mdn, { family: 4 });
    console.log(`[mdns] resolved ${mdn} → ${address}`);
    return address;
  } catch {
    console.log(`[mdns] ${host}.local not found`);
    return null;
  }
}
