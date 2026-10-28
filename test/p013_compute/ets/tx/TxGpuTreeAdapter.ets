// Copied/imported only when the T211 core module is actually present.
import { TxWholePairAdapter } from '../txcore/TxWholePairAdapter';
import { P011ParticleSnapshot } from '../P011ParticleField';
import { P011ParticleHotWorkspace } from '../P011ParticleHot';
import { TxPairResult,TxTreeBackend } from './TxPairBackends';
export class TxGpuTreeAdapter implements TxTreeBackend {
  private readonly core:TxWholePairAdapter;private readonly out:TxPairResult=new TxPairResult();
  constructor(nmax:number){this.core=new TxWholePairAdapter(nmax);}
  selectPair(p:P011ParticleSnapshot,radius:number,w:P011ParticleHotWorkspace):TxPairResult {
    const r=this.core.selectPair(p,radius,w);this.out.status=r.status;this.out.slotA=r.slotA;this.out.slotB=r.slotB;this.out.errorCode=r.errorCode;return this.out;
  }
}
