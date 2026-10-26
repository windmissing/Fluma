#include "scene_compositor.h"
namespace p013 {
MaterialParams SceneConfiguration::Params()const {
 MaterialParams p;for(int i=0;i<4;i++)p.canvasBox[i]=values[6+i];
 p.canvasToSurfaceScaleX=p.canvasToSurfaceScaleY=values[10];
 for(int i=0;i<9;i++)p.grainBaseSrgb[i]=values[20+i];for(int i=0;i<6;i++)p.paletteBaseSrgb[i]=values[29+i];
 p.effectiveLambdaMaterial=values[13];p.effectiveLambdaCanvasPx=values[14];p.warpRangeCycles=values[15];
 p.grainEnabled=values[16]!=0;p.strataEnabled=values[17]!=0;p.sparkEnabled=values[18]!=0;return p;
}
LightParams SceneConfiguration::Light()const {LightParams l;double* fields[]={&l.la,&l.ld,&l.lmin,&l.lmax,&l.lref,&l.ellLightMaterial,&l.lightX,&l.lightY,&l.lightZ,&l.sigmaCandidate};for(int i=0;i<10;i++)*fields[i]=values[35+i];return l;}
bool SceneCompositor::Register(const SceneConfiguration& c){
 if(!c.bindingId||c.digest.size()!=64)return false;
 for(double v:c.values)if(!std::isfinite(v)||!std::isfinite(float(v)))return false;
 if(c.values[0]<0||c.values[0]>2||std::floor(c.values[0])!=c.values[0]||c.values[1]<0||c.values[1]>3||std::floor(c.values[1])!=c.values[1]||c.values[2]<0||c.values[2]>9||std::floor(c.values[2])!=c.values[2]||!SafeInteger(c.values[3])||!SafeInteger(c.values[4])||c.values[5]!=double(c.revision)||c.values[8]<=0||c.values[9]<=0||c.values[10]<=0||c.values[11]<.5||c.values[11]>1||c.values[12]<.5||c.values[12]>1.25||c.values[19]<=0)return false;
 for(size_t i=20;i<35;i++)if(c.values[i]<0||c.values[i]>1)return false;
 for(size_t i=16;i<19;i++)if(c.values[i]!=0&&c.values[i]!=1)return false;
 if(c.values[13]<=0||c.values[14]<=0||c.values[15]!=.25||!SafeInteger(c.values[50]))return false;
 if(c.values[50]==20261026){const std::array<double,6> source{145,168,111,216,196,150};
  for(size_t i=0;i<6;i++)if(c.values[51+i]!=source[i]||c.values[29+i]!=source[i]/255.)return false;}
 auto light=c.Light();FrameInput check;check.frameId=check.sessionId=1;check.surfaceWidthPx=check.surfaceHeightPx=check.boxWidthPx=check.boxHeightPx=128;check.capacity=256;check.T_dataLayoutVersion=check.zOriginVersion=1;std::array<float,512> empty{};
 if(Validate(check,light,empty)!=Status::OK)return false;
 if(desired_&&desired_->bindingId==c.bindingId)return desired_->digest==c.digest&&desired_->values==c.values;
 desired_=std::make_shared<const SceneConfiguration>(c);return true;
}
SceneSource SceneCompositor::Source(const ScenePacket& p,uint64_t generation,uint64_t revision)const {
 SceneSource s;s.frameId=uint64_t(p.frame.frameId);s.fieldEpoch=uint64_t(p.frame.fieldEpoch);s.lightRevision=uint64_t(p.frame.lightConfigRevision);s.route=p.route;s.medium=p.medium;s.token=p.token;s.run=p.run;s.ledgerKey=p.ledgerKey;s.generation=generation;s.revision=revision;s.context=p.contextEpoch;s.surface=p.frame.surfaceEpoch;s.geometry=p.frame.geometryEpoch;s.continuity=p.frame.continuityEpoch;s.stateEpoch=p.stateEpoch;s.commit=p.commit;s.width=int(p.frame.surfaceWidthPx);s.height=int(p.frame.surfaceHeightPx);s.box={p.frame.boxXPx,p.frame.boxYPx,p.frame.boxWidthPx,p.frame.boxHeightPx};s.invert=p.frame.visualInverted;s.pipeline=desired_?(desired_->values[0]==0?"BASIC_FULLSCENE_V1":desired_->values[0]==1?(p.baseStripV2?"P1_FULLSCENE_OFF_V2_BASE_STRIP":"P1_FULLSCENE_OFF_V1"):(p.baseStripV2?"P1_FULLSCENE_ON_V2_BASE_STRIP":"P1_FULLSCENE_ON_V1")):"UNKNOWN";if(p.baseStripV2){s.stripSerial=p.baseStrip.serial;s.stripProfileHash=p.baseStrip.profileHash;s.stripPayloadHash=p.baseStrip.payloadHash;}return s;
}
bool SceneCompositor::Compatible(const ScenePacket& p)const {
 return valid_&&last_.context==p.contextEpoch&&last_.surface==p.frame.surfaceEpoch&&last_.geometry==p.frame.geometryEpoch&&last_.continuity==p.frame.continuityEpoch&&last_.width==p.frame.surfaceWidthPx&&last_.height==p.frame.surfaceHeightPx&&last_.box==std::array<double,4>{p.frame.boxXPx,p.frame.boxYPx,p.frame.boxWidthPx,p.frame.boxHeightPx}&&last_.invert==p.frame.visualInverted&&last_.run==p.run;
}
SceneReceipt SceneCompositor::Reject(const ScenePacket& p,int code,const char* stage,SceneGpu* gpu,bool replay){
 SceneReceipt r;r.requested=Source(p,p.requestedGeneration,p.frame.appearanceConfigRevision);r.lastGood=valid_?last_:SceneSource{};r.code=code;r.fault=stage;r.stage=stage;r.state="FAULT";
 if(replay&&gpu&&Compatible(p)){
  if(gpu->Present(true)==Status::OK&&gpu->Swap(uint64_t(p.frame.frameId))==Status::OK){r.staleSwap=true;r.state="STALE";r.actualGeneration=last_.generation;}
 }
 return r;
}
SceneReceipt SceneCompositor::Draw(const ScenePacket& p,MaterialUploader& uploader,SceneGpu& gpu,BaseStripUploader* strip){
 auto reject=[&](int c,const char* s,bool replay=true){return Reject(p,c,s,&gpu,replay);};
 auto bound=uploader.Inspect();
 if(!desired_||p.frame.appearanceConfigRevision!=desired_->revision||desired_->bindingId!=bound.bindingId||desired_->revision!=bound.boundRevision)return reject(402,"SCENE_CONFIG_BINDING");
 if(!SafeInteger(p.frame.frameId)||p.frame.frameId<=lastFrame_||p.token.empty()||p.run.empty()||!p.stateEpoch||!p.commit)return reject(102,"SCENE_STALE_TOKEN");
 lastFrame_=uint64_t(p.frame.frameId);
 if(valid_&&last_.run==p.run&&(p.stateEpoch<last_.stateEpoch||(p.stateEpoch==last_.stateEpoch&&p.commit<last_.commit)))return reject(102,"SCENE_STALE_SOURCE");
 if(valid_&&!Compatible(p))valid_=false;
 auto config=desired_;SceneLease lease;BaseStripLease baseLease;
 struct LeaseGuard {MaterialUploader& uploader;SceneLease& lease;BaseStripUploader* strip;BaseStripLease& base;
  ~LeaseGuard(){if(lease.desc)uploader.AbortSceneLease(lease,"exception");if(strip&&base.desc)strip->Abort(base);}} guard{uploader,lease,strip,baseLease};
 uint64_t generation=0;
 if(config->Material()){
  auto info=uploader.Inspect();generation=info.pendingGeneration&&uploader.CanActivate(p.requestedGeneration,p.contextEpoch)?p.requestedGeneration:info.activeGeneration;
  const MaterialDesc* desc=generation==info.pendingGeneration?uploader.PendingDesc():uploader.ActiveDesc();
  if(!desc||!generation)return reject(401,"SCENE_ASSET_NOT_READY");
  if(generation!=p.requestedGeneration){if(p.baseStripV2)return reject(401,"BASE_STRIP_MATERIAL_NOT_READY");config=active_;if(!config||!config->Material())return reject(401,"SCENE_ACTIVE_CONFIG_MISSING");}
  if(!uploader.AcquireSceneLease(generation,config->bindingId,p.token,uint64_t(p.frame.surfaceEpoch),p.contextEpoch,lease))return reject(401,"SCENE_LEASE_NOT_READY");
  if(lease.bindingDigest!=config->digest){uploader.AbortSceneLease(lease,"digest");return reject(402,"SCENE_CONFIG_DIGEST");}
 }
 auto abort=[&](int code,const char* stage){uploader.AbortSceneLease(lease,stage);if(strip)strip->Abort(baseLease);return reject(code,stage);};
 if(config->values[4]!=p.frame.geometryEpoch)return abort(103,"SCENE_CONFIG_GEOMETRY");
 if(p.baseStripV2){
  const auto& d=p.baseStrip;
  if(config->values[50]!=20261026)return abort(402,"BASE_PALETTE_CONFIG_REVISION");
  const std::array<double,6> encodedPalette{145,168,111,216,196,150};
  if(d.paletteHash!=MaterialSha256(reinterpret_cast<const uint8_t*>(encodedPalette.data()),sizeof(encodedPalette)))
   return abort(402,"BASE_PALETTE_PROVENANCE");
  for(size_t i=0;i<6;i++)if(config->values[29+i]!=encodedPalette[i]/255.)
   return abort(402,"BASE_PALETTE_PROVENANCE");
  if(!strip||!config->Material()||d.session!=p.frame.sessionId||d.token!=p.token||d.run!=p.run||
   d.stateEpoch!=p.stateEpoch||d.commit!=p.commit||d.bindingId!=config->bindingId||
   d.digest!=config->digest||d.geometry!=uint64_t(p.frame.geometryEpoch)||
   d.context!=p.contextEpoch||d.surface!=uint64_t(p.frame.surfaceEpoch)||
   d.width!=int(p.frame.surfaceWidthPx)||d.height!=int(p.frame.surfaceHeightPx)||
   d.originX!=0||d.originY!=0||d.boxX!=p.frame.boxXPx||d.boxWidth!=p.frame.boxWidthPx||
   d.mirror!=p.frame.visualInverted||d.spark!=(config->values[18]!=0))
   return abort(402,"BASE_STRIP_SCENE_IDENTITY");
  if(!strip->Acquire(d,baseLease))return abort(401,"BASE_STRIP_NOT_READY");
 }
 // All allocations/identity copies are completed before candidate or visible state is changed.
 SceneSource candidate=Source(p,generation,config->revision);candidate.lightRevision=config->revision;candidate.pipeline=config->values[0]==0?"BASIC_FULLSCENE_V1":config->values[0]==1?(p.baseStripV2?"P1_FULLSCENE_OFF_V2_BASE_STRIP":"P1_FULLSCENE_OFF_V1"):(p.baseStripV2?"P1_FULLSCENE_ON_V2_BASE_STRIP":"P1_FULLSCENE_ON_V1");
 gpu.SetBaseStripTexture(baseLease.texture);
 auto status=gpu.Begin(p);if(status!=Status::OK)return abort(int(status),gpu.FailureStage());
 const char* stages[]={"BACKGROUND","WATER_FOG","SEDIMENT","PARTICLES","HIGHLIGHT","INTERFACE_FOG","FOREGROUND"};
 for(int layer=0;layer<7;layer++) {status=gpu.Layer(layer,p,*config,lease.desc?&lease:nullptr);if(status!=Status::OK)return abort(int(status),stages[layer]);}
 status=gpu.Present(false);if(status!=Status::OK)return abort(int(status),"SCENE_PRESENT");
 status=gpu.Swap(uint64_t(p.frame.frameId));if(status!=Status::OK){uploader.AbortSceneLease(lease,"swap");return reject(int(status),"SCENE_SWAP",false);}
 // No JS callback, allocation, or mutable configuration read between swap and these commits.
 if(lease.desc)uploader.CommitSceneLeaseAfterSwap(lease);
 if(baseLease.desc)strip->CommitAfterSwap(baseLease);
 gpu.CommitTargets();last_=std::move(candidate);valid_=true;active_=std::move(config);
 SceneReceipt r;r.code=0;r.state="CURRENT";r.fault=r.stage="NONE";r.current=r.requestedSwap=true;r.requested=Source(p,p.requestedGeneration,p.frame.appearanceConfigRevision);r.rendered=r.lastGood=last_;r.actualGeneration=generation;r.actualStripSerial=last_.stripSerial;return r;
}
}
