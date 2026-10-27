import { P011ParticleSnapshot } from './P011ParticleField';
import { P011ParticleHotWorkspace } from './P011ParticleHot';
export interface T159Pair { status: string; slotA: number; slotB: number; errorCode: string; }
export interface T159Binding { selectPair(p: P011ParticleSnapshot, radius: number, work: P011ParticleHotWorkspace): T159Pair; }
let route = 'UNARMED';
let binding: T159Binding | null = null;
export function t159Arm(next: string, native: T159Binding | null): void {
  if (route !== 'UNARMED' || (next !== 'GPU' && next !== 'T123' && next !== 'LEGACY_SCAN') ||
    (next === 'GPU' && native === null)) throw new Error('ARM_INVALID');
  binding = native; route = next;
}
export function t159Disarm(): void { route = 'UNARMED'; binding = null; }
export function t159Select(p: P011ParticleSnapshot, radius: number, w: P011ParticleHotWorkspace): T159Pair {
  if (route === 'GPU') {
    if (binding === null) throw new Error('NATIVE_UNAVAILABLE');
    return binding.selectPair(p,radius,w); // Never fall through after any native error.
  }
  if (route === 'T123') {
    const found = w.mergeIndex.find(p,radius);
    return {status: found ? 'PAIR' : 'NO_PAIR',slotA:w.mergeIndex.selectedA,slotB:w.mergeIndex.selectedB,errorCode:'NONE'};
  }
  if (route !== 'LEGACY_SCAN') throw new Error('NOT_ARMED');
  let after = 0;
  for (let pass = 0; pass < p.nmax; pass++) {
    let a = -1; let aid = Infinity;
    for (let i = 0; i < p.nmax; i++) if (p.pState[i] !== 0 && p.pId[i] > after && p.pId[i] < aid) { a=i; aid=p.pId[i]; }
    if (a < 0) break;
    let b = -1; let bid = Infinity;
    for (let j = 0; j < p.nmax; j++) if (p.pState[j] !== 0 && p.pId[j] > aid && p.pId[j] < bid &&
      p.pPhase[a] === p.pPhase[j] && p.pSource[a] === p.pSource[j] && p.pSourceId[a] === p.pSourceId[j] &&
      p.pState[a] === p.pState[j] && Math.floor(p.pX[a]) === Math.floor(p.pX[j]) && Math.abs(p.pY[a]-p.pY[j]) <= radius) { b=j; bid=p.pId[j]; }
    if (b >= 0) return {status:'PAIR',slotA:a,slotB:b,errorCode:'NONE'};
    after=aid;
  }
  return {status:'NO_PAIR',slotA:-1,slotB:-1,errorCode:'NONE'};
}
