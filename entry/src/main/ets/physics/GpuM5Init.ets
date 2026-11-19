import native from 'libgrsm1.so';
import util from '@ohos.util';
import { DiagnosticColdFiles } from './DiagnosticColdFiles';
interface InitPage { revision: string; state: string; ticket: string; boot: string; page: number; snapshotRevision: string; base64: string; }
// Initialization observations only. Never fabricates scope, input, pulse, Ready or native failure.
export class GpuM5Init {
 private static pair(id: string): number[] {let lo: number = 0;let hi: number = 0;for(let i: number = 0;i<id.length;i++){const next: number=lo*10+Number(id.charAt(i));hi=hi*10+Math.floor(next/4294967296);lo=next%4294967296;if(hi>4294967295)throw new Error('TICKET_OVERFLOW');}return [lo,hi];}
 static ticket(raw: string): string {
  if (!raw || raw.length > 4096) throw new Error('CONFIG_REPLY_BOUND');
  const value = JSON.parse(raw) as Record<string, Object>;
  const ticket: Object = value['requestTicket'];
  if (value['state'] !== 'M5_NEW_EMPTY_RUN_REQUESTED' || typeof ticket !== 'string' || !/^[1-9][0-9]{0,19}$/.test(ticket)) throw new Error('CONFIG_TICKET_MISSING');
  return ticket;
 }
 static async archive(ticket: string, directory: string, client: string): Promise<string> {
  const pages: string[] = []; let revision: string = ''; let complete: boolean = true;
  for (let page: number = 0; page < 8; page++) {
   const raw: string = native.m5InitDiagnostics(ticket, page);
   if (raw.length > 4096) throw new Error('INIT_PAGE_BOUND');
   const item = JSON.parse(raw) as InitPage;
   if (item.state !== 'SNAPSHOT_PAGE' || item.ticket !== ticket || item.page !== page) { complete = false; pages.push(raw); break; }
   if (page === 0) revision = item.snapshotRevision; else if (revision !== item.snapshotRevision) complete = false;
   pages.push(raw);
  }
  const text: string = JSON.stringify({ revision: 'T653_M5_INIT_ARCHIVE_V1', ticket: ticket, completeSnapshot: complete, client: client, pages: pages });
  const stored = await new DiagnosticColdFiles(directory).save('m5-init-' + ticket + '-' + Date.now() + '.json', text);
  return (complete ? 'OBSERVATION' : 'INCOMPLETE_REVISION') + ':' + stored.name + ':' + stored.sha256;
 }
 static wait(ticket: string, live: () => boolean, ready: (identity: number[], source: number[]) => void, fault: (message: string) => void): void {
  let polls: number = 0; const started: number = Date.now();
  const poll = (): void => {
   if (!live()) return;
   try {
    const raw: string = native.m5Session();
    if (!raw || raw.length > 4096) throw new Error('SESSION_BOUND');
    const session = JSON.parse(raw) as Record<string, Object>;
    const identity: Object = session['identity']; const source: Object = session['sourceControlGeneration'];
    if (session['revision'] === 5 && session['features'] === 31 && Array.isArray(identity) && Array.isArray(source)) {
     if (session['requestTicket'] !== ticket || identity.length !== 18 || source.length !== 2) throw new Error('SESSION_TICKET_SCOPE');
     const ids: number[] = []; const generation: number[] = [];
     for (let i: number = 0; i < identity.length; i++) { const word: Object = identity[i]; if (typeof word !== 'number' || !Number.isSafeInteger(word) || word < 0 || word > 4294967295) throw new Error('SESSION_WORD'); ids.push(word); }
     for (let i: number = 0; i < 2; i++) { const word: Object = source[i]; if (typeof word !== 'number' || !Number.isSafeInteger(word) || word < 0 || word > 4294967295) throw new Error('SESSION_SOURCE'); generation.push(word); }
     for (let i: number = 0; i < 18; i += 2) if (ids[i] === 0 && ids[i + 1] === 0) throw new Error('SESSION_UNKNOWN_SCOPE');
     if (generation[0] === 0 && generation[1] === 0) throw new Error('SOURCE_GENERATION_UNKNOWN');
     if (live()) ready(ids, generation); return;
    }
    // One bounded page per poll reveals a real per-ticket first failure, independent of the GL owner.
    const page = JSON.parse(native.m5InitDiagnostics(ticket, polls % 2)) as InitPage;
    if (page.state === 'STALE_TICKET') throw new Error('STALE_TICKET');
    if (page.state !== 'SNAPSHOT_PAGE' || page.ticket !== ticket || page.base64.length > 2732) throw new Error('INIT_DIAGNOSTICS_INCOMPLETE');
    const bytes: Uint8Array = new util.Base64Helper().decodeSync(page.base64); if (bytes.length !== 2048) throw new Error('INIT_PAYLOAD_BOUND');
    const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    for (let slot: number = 0; slot < 2; slot++) {
     const offset: number = (page.page === 0 ? 1024 : 0) + slot * 512;
     const lo: number = view.getUint32(offset + 16, true); const hi: number = view.getUint32(offset + 20, true);
     const pair: number[] = GpuM5Init.pair(ticket);
     if (pair[0] === lo && pair[1] === hi) {
      const terminal: number = view.getUint32(offset + 107 * 4, true);
      if (terminal !== 0) throw new Error('NATIVE_INIT_TERMINAL:' + terminal + ':stage=' + view.getUint32(offset + 106 * 4, true) + ':code=' + view.getInt32(offset + 105 * 4, true));
     }
    }
    if (++polls >= 450 || Date.now() - started >= 15000) { if (live()) fault('INIT_OBSERVATION_TIMEOUT_NATIVE_STATE_UNKNOWN:lastSession=' + raw); return; }
    setTimeout(poll, 33);
   } catch (error) { if (live()) fault('M5_INIT:' + String(error)); }
  };
  poll();
 }
}
