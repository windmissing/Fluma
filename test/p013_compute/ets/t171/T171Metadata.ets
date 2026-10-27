// Generated-test metadata only. No changes to authoritative arithmetic.
import { P011ParticleSnapshot } from './P011ParticleField';
export class T159Words { lo: number = 0; hi: number = 0; }
export function t159Increment(v: T159Words): void {
  if (v.lo === 4294967295) {
    if (v.hi === 4294967295) throw new Error('SERIAL_EXHAUSTED');
    v.lo = 0; v.hi++;
  } else v.lo++;
}
export function t159Split(value: number): T159Words {
  if (!Number.isSafeInteger(value) || value < 0) throw new Error('ORIGINAL_ID_INVALID');
  const v = new T159Words(); v.lo = value % 4294967296; v.hi = Math.floor(value / 4294967296); return v;
}
let clock = new T159Words();
let versions: Map<P011ParticleSnapshot, T159Words> = new Map();
let last: P011ParticleSnapshot | null = null;
let role = 0;
let ordinals: number[] = [0,0,0,0,0,0];
export function t159Role(next: number): void {
  if (next < 1 || next > 5 || !Number.isInteger(next)) throw new Error('ROLE_INVALID');
  if (ordinals[next] === 4294967295) throw new Error('ORDINAL_EXHAUSTED');
  role = next; ordinals[next]++;
}
export function t159CandidateRole(): number { if (role === 0) throw new Error('ROLE_MISSING'); return role; }
export function t159CandidateOrdinal(): number { return ordinals[t159CandidateRole()]; }
export function t159Mutation(p: P011ParticleSnapshot): void {
  if (!versions.has(p) && versions.size >= 64) throw new Error('TEST_POOL_METADATA_LIMIT');
  t159Increment(clock); const v = new T159Words(); v.lo = clock.lo; v.hi = clock.hi; versions.set(p,v);
}
export function t159PoolVersion(p: P011ParticleSnapshot): T159Words {
  if (last !== p || !versions.has(p)) t159Mutation(p);
  last = p;
  return versions.get(p)!;
}
export function t159ResetMetadata(): void {
  clock = new T159Words(); versions.clear(); last = null; role = 0; ordinals = [0,0,0,0,0,0];attemptBase=[0,0,0,0,0,0];
}

let attemptBase:number[]=[0,0,0,0,0,0];
export function t171ResetAttemptMetadata():void{versions.clear();last=null;role=0;attemptBase=ordinals.slice();}

export function t171RoleCounts():number[]{const counts:number[]=[];for(let i=0;i<6;i++)counts.push(ordinals[i]-attemptBase[i]);return counts;}
export function t171RelativeOrdinal():number{return ordinals[t159CandidateRole()]-attemptBase[t159CandidateRole()];}
