// Test package only. Import this from the isolated test Ability, never production.
import gpuPair from 'libp011_gpu_pair_prototype.so';
import { registerT141PairBinding, T141Pair } from './P011GpuPairSelector';
import { P011ParticleSnapshot } from './P011ParticleField';
type T141ConfigureReply = { status: string; errorCode: string };

export function armT141GpuPair(nmax: number, shaderDirectory: string, buildManifestSha256: string): string {
  const result = gpuPair.configure(nmax, shaderDirectory, buildManifestSha256) as T141ConfigureReply;
  if (result.status !== 'READY') return result.errorCode;
  registerT141PairBinding({selectPair(p: P011ParticleSnapshot, radius: number, work: Object): T141Pair {
    return gpuPair.selectPair(p, radius, work) as T141Pair;
  }});
  return 'READY';
}
export function disarmT141GpuPair(): void {
  registerT141PairBinding(null); gpuPair.reset();
}
