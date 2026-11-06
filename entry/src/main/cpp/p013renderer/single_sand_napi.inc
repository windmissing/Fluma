#if !defined(P013_MATERIAL_PASS_CORE_ONLY) && !defined(P013_SCENE_BRIDGE_TEST)
bool SingleText(napi_env e,napi_value o,const char* key,std::string& out){
 napi_value v=nullptr;
 char bytes[2049]{};size_t size=0;napi_valuetype type;
 if(napi_get_named_property(e,o,key,&v)!=napi_ok||napi_typeof(e,v,&type)!=napi_ok||type!=napi_string||
    napi_get_value_string_utf8(e,v,bytes,sizeof(bytes),&size)!=napi_ok||size>=sizeof(bytes)-1)return false;
 out.assign(bytes,size);return true;
}
bool SingleU64(napi_env e,napi_value o,const char* key,uint64_t& out){
 double n=0;if(!Number(e,o,key,n)||!SafeInteger(n))return false;out=uint64_t(n);return true;
}
napi_value SingleResult(napi_env e,const SingleSandReceipt& r,const SingleSandPacket& p){
 napi_value o=nullptr;napi_create_object(e,&o);
 Put(e,o,"revision","P013_SINGLE_SAND_RECEIPT_V1");Put(e,o,"reason",r.reason.c_str());
 Put(e,o,"presentationState",r.state.c_str());Put(e,o,"firstFailureStage",r.stage);
 Put(e,o,"requestedToken",p.token.c_str());Put(e,o,"actualToken",r.actual.c_str());
 Put(e,o,"actualConfigDigest",r.actualDigest.c_str());Put(e,o,"actualGrainHash",r.actualGrain.c_str());
 Put(e,o,"actualProfileHash",r.actualProfile.c_str());Put(e,o,"actualPbHash",r.actualPb.c_str());
 Put(e,o,"requestedConfigDigest",p.digest.c_str());Put(e,o,"requestedGrainHash",p.grainHash.c_str());
 Put(e,o,"rawSourceHash",p.rawHash.c_str());Put(e,o,"displayProfileHash",p.displayHash.c_str());Put(e,o,"pbHash",p.pbHash.c_str());
 Put(e,o,"shaderHash",SingleSandShaderHash());Put(e,o,"currentFrameSucceeded",r.state=="CURRENT");
 Put(e,o,"swapSucceeded",r.swapped);Put(e,o,"rawGL",double(r.rawGl));Put(e,o,"rawEGL",double(r.rawEgl));
 Put(e,o,"staleSwapSucceeded",r.replayed);Put(e,o,"drawCalls",double(r.drawCalls));Put(e,o,"drawWallMs",r.wallMs);
 Put(e,o,"textureCount",double(r.textureCount));Put(e,o,"occludedCentersCpu",double(r.occludedCenters));
 Put(e,o,"bedSupportCpu",r.bedSupport);Put(e,o,"assetLease",r.lease);
 Put(e,o,"profileGpuBytes",1024.);Put(e,o,"instanceGpuBytes",8192.);Put(e,o,"assetCpuBytes",131072.);
 Put(e,o,"runId",p.run.c_str());Put(e,o,"publicationKind",p.kind.c_str());Put(e,o,"physicsCommitId",double(p.commit));
 Put(e,o,"stateEpoch",double(p.epoch));Put(e,o,"surfaceEpoch",double(p.surface));Put(e,o,"contextEpoch",double(p.context));
 Put(e,o,"certificate",double(p.certificate));Put(e,o,"sealSerial",double(p.seal));
 Put(e,o,"geometryEpoch",double(p.geometry));Put(e,o,"continuityEpoch",double(p.continuity));
 Put(e,o,"attachmentEncoding",double(r.encoding));Put(e,o,"actualColorSpace",double(r.colorSpace));
 Put(e,o,"samples",double(r.samples));Put(e,o,"fragmentHighp",double(r.highp));
 Put(e,o,"writeControlSupported",r.writeControl);Put(e,o,"srgbWriteEnabled",r.writeEnabled);
 Put(e,o,"uploadedBytes",double(r.uploaded));Put(e,o,"targetBytes",double(r.targets));Put(e,o,"readbackBytes",0.);
 Put(e,o,"count",double(p.count));Put(e,o,"generation",double(p.generation));
 return o;
}
napi_value RegisterSingleSand(napi_env e,napi_callback_info info){
 size_t n=2;napi_value a[2]{};napi_get_cb_info(e,info,&n,a,nullptr,nullptr);
 std::lock_guard<std::mutex> lock(g.mutex);SingleSandPacket p;SingleSandReceipt r;SingleSandConfig c;
 const char* stage="CONFIG";void* values=nullptr;
 r.reason="SINGLE_CONFIG_INVALID";r.stage=stage;std::string versions;
 if(n==2&&MaterialOwner()==Status::OK&&SingleText(e,a[0],"versions",versions)&&versions==SingleSandVersions&&
    SingleU64(e,a[0],"session",c.session)&&c.session==g.id&&SingleU64(e,a[0],"revision",c.revision)&&
    SingleText(e,a[0],"digest",c.digest)&&SingleText(e,a[0],"grainHash",c.grainHash)&&
    SingleText(e,a[0],"generatorHash",c.generatorHash)&&SceneDoubleView(e,a[1],19,values)){
  std::memcpy(c.values.data(),values,sizeof(c.values));r.reason=c.Validate();
  if(r.reason=="OK"){
   auto& old=g.singleSand.pendingConfig;
   if(old.session==c.session&&(c.revision<old.revision||(c.revision==old.revision&&c.digest!=old.digest)))r.reason="SINGLE_CONFIG_ASSET_MISMATCH";
   else{if(old.session!=c.session)g.singleSand.lastGeneration=0;old=std::move(c);}
  }
 }
 return SingleResult(e,r,p);
}
napi_value StageSingleSand(napi_env e,napi_callback_info info){
 size_t n=2;napi_value a[2]{};napi_get_cb_info(e,info,&n,a,nullptr,nullptr);
 std::lock_guard<std::mutex> lock(g.mutex);SingleSandPacket p;SingleSandReceipt r;
 void* bytes=nullptr;const char* stage="R8_STAGE";r.stage=stage;r.reason="SINGLE_ASSET_HASH";
 if(n==2&&MaterialOwner()==Status::OK&&ExactString(e,a[0],"asset","SINGLE_GRAIN_R8_SIGNED_V1")&&
    SingleU64(e,a[0],"session",p.session)&&p.session==g.id&&SingleU64(e,a[0],"generation",p.generation)&&
    SingleText(e,a[0],"digest",p.digest)&&Uint8View(e,a[1],65536,bytes))
  r.reason=g.singleSand.Stage(p.generation,p.digest,static_cast<const uint8_t*>(bytes));
 return SingleResult(e,r,p);
}
napi_value DrawSingleSand(napi_env e,napi_callback_info info){
 size_t n=4;napi_value a[4]{};napi_get_cb_info(e,info,&n,a,nullptr,nullptr);
 std::lock_guard<std::mutex> lock(g.mutex);SingleSandPacket p;SingleSandReceipt r;
 if(n!=4||!ExactString(e,a[0],"packet","P013_SINGLE_SAND_SCENE_PACKET_V1")||
    !ExactString(e,a[0],"pipeline","SINGLE_SAND_DUALSTATE_SCENE_V1")||
    !ExactString(e,a[0],"projection","COLUMN_PRESERVING_F32_RENDER_V1")){
  r.reason="SINGLE_VERSION_UNSUPPORTED";return SingleResult(e,r,p);
 }
 if(!ExactString(e,a[0],"material","SINGLE_H_SAND_V1")||!ExactString(e,a[0],"medium","DRY_AIR_V1")||
    !ExactString(e,a[0],"route","dual-field-particle-v1")){
  r.reason="UNSUPPORTED_MATERIAL_SET";return SingleResult(e,r,p);
 }
 const auto u=[&](const char* k,uint64_t& v){return SingleU64(e,a[0],k,v);};
 const auto s=[&](const char* k,std::string& v){return SingleText(e,a[0],k,v);};
 const auto d=[&](const char* k,double& v){return Number(e,a[0],k,v);};
 uint64_t width=0,height=0,count=0,slots=0;
 if(!u("session",p.session)||!u("surface",p.surface)||!u("context",p.context)||!u("geometry",p.geometry)||
    !u("continuity",p.continuity)||!u("epoch",p.epoch)||!u("commit",p.commit)||!u("frame",p.frame)||
    !u("certificate",p.certificate)||!u("seal",p.seal)||!u("revision",p.revision)||!u("generation",p.generation)||
    !u("width",width)||!u("height",height)||width>INT32_MAX||height>INT32_MAX||!u("count",count)||!u("slots",slots)||
    slots<2||slots>512||count>slots||!s("run",p.run)||!s("token",p.token)||!s("kind",p.kind)||!s("ledger",p.ledger)||
    !s("digest",p.digest)||!s("grainHash",p.grainHash)||!s("rawHash",p.rawHash)||!s("displayHash",p.displayHash)||
    !s("pbHash",p.pbHash)||!s("particleHash",p.particleHash)||!s("shaderHash",p.shaderHash)||!d("logicalW",p.logicalW)||!d("logicalH",p.logicalH)||
    !d("capacity",p.capacity)||!d("x",p.box[0])||!d("y",p.box[1])||!d("w",p.box[2])||!d("h",p.box[3])||
    !Boolean(e,a[0],"inverted",p.inverted)||!Boolean(e,a[0],"enabled",p.enabled)){
  r.reason="SINGLE_TYPED_RANGE";return SingleResult(e,r,p);
 }
 p.width=int(width);p.height=int(height);p.slots=int(slots);p.count=int(count);
 void *profile=nullptr,*xy=nullptr,*meta=nullptr;const char* stage="TYPED_COPY";
 if(!TypedView(e,a[1],napi_float32_array,256,profile,stage)||
    !TypedView(e,a[2],napi_float32_array,slots*2,xy,stage)||!TypedView(e,a[3],napi_uint32_array,slots*2,meta,stage)){
  r.reason="SINGLE_TYPED_RANGE";return SingleResult(e,r,p);
 }
 const auto overlap=[](void* a,size_t an,void* b,size_t bn){
  auto x=reinterpret_cast<uintptr_t>(a),y=reinterpret_cast<uintptr_t>(b);
  return x<=y?y-x<an:x-y<bn;
 };
 if(overlap(profile,1024,xy,slots*8)||overlap(profile,1024,meta,slots*8)||overlap(xy,slots*8,meta,slots*8)){
  r.reason="SINGLE_TYPED_RANGE";r.stage="TYPED_ALIAS";return SingleResult(e,r,p);
 }
 std::memcpy(p.profile.data(),profile,1024);std::memcpy(p.xy.data(),xy,count*8);std::memcpy(p.meta.data(),meta,count*8);
 r.reason=p.Validate();if(r.reason!="OK")return SingleResult(e,r,p);
 if(p.shaderHash!=SingleSandShaderHash()){
  r.reason="SINGLE_VERSION_UNSUPPORTED";return SingleResult(e,r,p);
 }
 if(MaterialOwner()!=Status::OK||p.session!=g.id||p.context!=g.egl.contextEpoch||p.surface!=g.egl.surfaceEpoch){
  r.reason="SINGLE_PUBLICATION_MISMATCH";return SingleResult(e,r,p);
 }
 SurfaceInfo surface{};auto current=g.egl.EnsureCurrent({p.surface,p.width,p.height},surface);
 if(current!=Status::OK){r.reason="SINGLE_TARGET_UNSUPPORTED";r.stage=g.egl.errorStage;r.rawEgl=g.egl.error;return SingleResult(e,r,p);}
 const double begin=NowMs();
 try{r=g.singleSand.Draw(g.egl,p);}catch(const std::bad_alloc&){r.reason="SINGLE_DRAW_FAILED";r.stage="CPU_ALLOCATION";}
 r.wallMs=NowMs()-begin;
 if(g.egl.Unbind()!=Status::OK){if(r.reason=="OK"){r.reason="SINGLE_DRAW_FAILED";r.stage="UNBIND";}r.state="FAULT";r.rawEgl=g.egl.error;}
 return SingleResult(e,r,p);
}
#endif
