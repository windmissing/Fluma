import nativeRenderer from 'libp013renderer.so';
import { P011PublishedRenderState } from '../physics/P011PublishedRenderState';
import { P012FrameContext } from './P012RenderTypes';
import { P013SingleSandPreparation } from './P013SingleSandPreparation';
import { SingleSandConfigV1,SingleSandGrainV1,SingleSandNative,SingleSandReceipt } from './P013SingleSandTypes';
import { SINGLE_GENERATOR_HASH,buildSingleSandGrain } from './preparation/P013SingleSandAssets';
import { p013Sha256 } from './preparation/P013CanvasAssetExport';

export function singleSandConfigDigest(c:SingleSandConfigV1,values:Float64Array):string {
  const parts:string[]=[c.versions,String(c.revision),c.generatorHash,c.grainHash];
  let canonical='';
  for(const part of parts)canonical+=String(part.length)+':'+part;
  const data=new DataView(new ArrayBuffer(8));
  const hex='0123456789abcdef';
  for(let i=0;i<values.length;i++){
    data.setFloat64(0,values[i],true);
    for(let j=0;j<8;j++){const byte=data.getUint8(j);canonical+=hex.charAt(byte>>4)+hex.charAt(byte&15);}
  }
  const bytes=new Uint8Array(canonical.length);
  for(let i=0;i<bytes.length;i++)bytes[i]=canonical.charCodeAt(i);
  return p013Sha256(bytes);
}

export class P013SingleSandController {
  readonly preparation:P013SingleSandPreparation=new P013SingleSandPreparation();
  private readonly config:SingleSandConfigV1=new SingleSandConfigV1();
  private readonly desc:SingleSandGrainV1=new SingleSandGrainV1();
  private readonly values:Float64Array=new Float64Array([145,168,111,16,16,36,2,2,.08,.10,8,-.35,0,.82,.10,.16,.60,.95,20261022]);
  private readonly native:SingleSandNative;
  private tile:Uint8Array|null=null;
  private session:number=0;
  private bound:boolean=false;
  private shaderHash:string='';
  failure:string='NOT_READY';
  lastReceipt:SingleSandReceipt|null=null;

  constructor(native:SingleSandNative|null=null){
    this.native=native===null?nativeRenderer as SingleSandNative:native;
  }
  prepareSingleSandOptics():boolean {
    if(this.bound)return true;
    if(this.tile===null){
      this.tile=buildSingleSandGrain(this.values[18],this.values[7]);
      this.config.grainHash=p013Sha256(this.tile);
      this.config.generatorHash=SINGLE_GENERATOR_HASH;
      this.config.digest=singleSandConfigDigest(this.config,this.values);
    }
    this.config.session=this.session;
    const configured=this.native.registerSingleSandConfiguration(this.config,this.values);
    if(configured.reason!=='OK'){this.failure=configured.reason;return false;}
    this.shaderHash=configured.shaderHash;
    this.desc.session=this.session;
    this.desc.digest=this.config.digest;
    this.desc.generation++;
    const staged=this.native.stageSingleSandGrain(this.desc,this.tile);
    this.failure=staged.reason;
    this.bound=staged.reason==='OK';
    return this.bound;
  }
  prepareSingleSandFrame(state:P011PublishedRenderState,context:P012FrameContext,scale:number):boolean {
    if(!Number.isFinite(scale)||scale<=0||!Number.isFinite(context.boxW)||context.boxW<=0||
       !Number.isFinite(context.boxH)||context.boxH<=0){
      this.failure='SINGLE_GEOMETRY_UNSUPPORTED';return false;
    }
    const p=this.preparation.packet;
    p.x=context.boxX*scale;p.y=context.boxY*scale;p.w=context.boxW*scale;p.h=context.boxH*scale;
    p.logicalW=context.boxW;p.logicalH=context.boxH;
    this.failure=this.preparation.pack(state,context);
    if(this.failure!=='OK')return false;
    if(this.session===0){
      const init=this.native.init('p013_lmat_surface');
      if(init.code!==0){this.failure='SINGLE_TARGET_UNSUPPORTED';return false;}
      this.session=init.sessionId;this.bound=false;
    }
    const target=this.native.getState();
    const ready=this.native.prepareDualFieldSurface(this.session,target.surfaceEpoch);
    if(ready.code!==0||ready.sizeSource!=='EGL_QUERY_SURFACE'){
      this.failure='SINGLE_TARGET_UNSUPPORTED';return false;
    }
    p.session=this.session;p.surface=ready.surfaceEpoch;p.context=ready.contextEpoch;
    p.width=ready.widthPx;p.height=ready.heightPx;
    if(!this.prepareSingleSandOptics())return false;
    p.revision=this.config.revision;p.digest=this.config.digest;
    p.generation=this.desc.generation;p.grainHash=this.config.grainHash;
    p.shaderHash=this.shaderHash;
    p.token=p.run+':'+p.epoch+':'+p.commit+':'+p.frame+':'+p.generation+':'+p.digest+':'+p.displayHash+':'+p.pbHash+':'+p.particleHash;
    return true;
  }
  drawSingleSandScene(state:P011PublishedRenderState,context:P012FrameContext,scale:number):SingleSandReceipt|null {
    if(!this.prepareSingleSandFrame(state,context,scale))return null;
    const prep=this.preparation;
    const receipt=this.native.drawSingleSandScene(prep.packet,prep.profile,prep.slot.particleXY,prep.slot.particleMeta);
    this.lastReceipt=receipt;
    this.failure=receipt.reason;
    return receipt;
  }
}
