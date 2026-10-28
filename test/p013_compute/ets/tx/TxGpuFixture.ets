import { WaterSandField } from '../WaterSandField';
import { defaultP011ParticleParams } from '../P011ParticleM2Field';
import { TxPairBackends,TxGpuConfig,TxNativeTransport,TxTreeBackend } from './TxPairBackends';
import { T171_FIXTURES } from '../T171Fixtures';
export interface TxGpuFixtureResult {revision:string;fixtureId:string;backend:string;physicalStatus:string;error:string;synthetic:boolean;physicalMs:number;ledger:Float64Array;states:string[];errors:string[];nativeRows:string[];before:string;after:string;retirement:string;configuration:string;firstError:string;count:number;resource:string;cpuInclusiveMs:Float64Array;cpuCalls:Uint32Array;}
export function runTxGpuFixture(config:TxGpuConfig,fixtureId:string,steps:number,transport:TxNativeTransport|null,clock:()=>number,tree:TxTreeBackend|null=null):TxGpuFixtureResult {
  const f=T171_FIXTURES.find((v):boolean=>v.fixtureId===fixtureId);if(f===undefined||!Number.isInteger(steps)||steps<1||steps>32||config.nmax!==f.nmax)throw new Error('TX_GPU_FIXTURE_INPUT');
  const field=new WaterSandField(128,512),params=defaultP011ParticleParams();params.nmax=f.nmax;
  if(JSON.stringify(params)!==JSON.stringify(f.parameters)||!field.setParticleRoute(params).ok)throw new Error('TX_GPU_FIXTURE_PARAMETERS');field.setNaturalObservationRouteId('dual-field-particle-v1');
  const backend=new TxPairBackends(config,transport);backend.clock=clock;if(tree!==null)backend.attachTree(tree);field.txAttachBackend(backend);
  const before=field.t171ArchiveState();let retirement='NOT_RUN',elapsed=NaN,error='';
  try{
    const start=clock();
    for(let input=0;input<2;input++){
      backend.beginRoot();const h=new Float64Array(128),l=new Float64Array(128);h[f.column]=(input===0?f.nmax:f.packages)*f.massPackage;
      const r=field.injectGestureSand({perColumnHeavy:h,perColumnLight:l,sourceId:f.sourceId,sequence:f.sequences[input],inputTime:f.inputTimes[input],x:f.x,y:f.y,seed:f.seed});
      if(r===null||Math.abs(r.admittedH-h[f.column])>1e-10)throw new Error('TX_GPU_ADMISSION_FAILED');
    }
    for(let n=0;n<steps;n++){backend.beginRoot();const r=field.step(1/60);if(!r.ok)throw new Error('TX_GPU_PHYSICAL_FAILED:'+String(r.errorCode));}
    elapsed=clock()-start;
  }catch(failure){error=(failure as Error).message;}finally{const r=backend.retire();retirement=JSON.stringify({state:backend.state,native:r});}
  return {revision:'TX_GPU_FIXTURE_RESULT_V1',fixtureId:fixtureId,backend:config.backend,physicalStatus:error.length?'FAULT':'COMMITTED',error:error,synthetic:true,physicalMs:elapsed,ledger:backend.rows.slice(0,backend.count*64),states:backend.states.slice(0,backend.count),errors:backend.errors.slice(0,backend.count),nativeRows:backend.nativeRows.slice(0,backend.count),before:before,after:field.t171ArchiveState(),cpuInclusiveMs:backend.cpuTotals.slice(),cpuCalls:backend.cpuCounts.slice(),retirement:retirement,configuration:backend.readyReport(),firstError:backend.firstError,count:backend.count,resource:JSON.stringify({revision:'TX_GPU_RESOURCE_V1',N:config.nmax,views:8,metadataBytes:8*12*8+6*4,cpuCountersBytes:48,adapterFrozenBytes:2*(55*config.nmax+16*128+13*4*2),traceBytes:config.traceCapacity*64*8,identityCapacityPerNativeSlot:12288,externalCopies:'UNBOUNDED_EXTERNAL',runtimeNativeDriverBytes:'NOT_TOTALLED',wrapperAllocationsPerSuccessfulGpuRequest:26})};
}
