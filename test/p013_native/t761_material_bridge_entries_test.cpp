#define P013_MATERIAL_BRIDGE_TEST
// T81: fresh per-entry coverage of the CURRENT material bridge entries
// (napi_bridge.cpp exports) against the real MaterialUploader/MaterialPass
// core, on the host NAPI stub. Same-instance sequencing that T75's core
// suite exercised only on an isolated second uploader:
//   READY pending -> bind-configuration refusal (402) on the SAME instance,
//   activate -> rebind -> READY-pending eligibility loss,
//   drawMaterialFrame full path after each bind state.
// Companion .cmd compiles with the same cl.exe recipe as T75's runner.
#include "bridge_host_test.cpp"
#include <cstdio>
using namespace p013;
// Num/Text/Call/Read and BooleanValue where present come from bridge_host_test.cpp.
static napi_value BooleanValue(bool x){napi_value v=nullptr;napi_get_boolean(env,x,&v);return v;}
static napi_value List(std::initializer_list<double> values){auto a=Value();for(auto x:values)a->elements.push_back(Num(x));return a;}
static std::string ReadText(napi_value v,const char* k){auto i=v->fields.find(k);return i==v->fields.end()?std::string():i->second->text;}
static void Set(napi_value v,const char* k,napi_value x){napi_set_named_property(env,v,k,x);}
template<class T> auto BindFixtureTarget(T& uploader,uint64_t revision,int) -> decltype(uploader.BindConfiguration(1,revision,std::string(64,'a')),bool()) {return uploader.BindConfiguration(1,revision,std::string(64,'a')).code==0;}
template<class T> bool BindFixtureTarget(T&,uint64_t,long){return true;}
int RunT761Entries(){
 // --- Same real init/session/surface path as production entries ---
 auto* component=reinterpret_cast<OH_NativeXComponent*>(1);g.component=component;g.componentId="t761-bridge";g.egl.OnWindowCreated(component,reinterpret_cast<void*>(1));
 auto init=Call(InitSession,{Text("t761-bridge")});CHECK(Read(init,"code")==0);
 auto prep=Call(PrepareDualFieldSurface,{Num(g.id),Num(g.egl.surfaceEpoch)});CHECK(Read(prep,"code")==0);
 // --- Real bytes, stage through the REAL bridge entry ---
 std::vector<uint8_t> grain(kMaterialGrainBytes,128),strata(kMaterialStrataBytes,128);
 for(size_t i=3;i<grain.size();i+=4)grain[i]=255;for(size_t i=3;i<strata.size();i+=4)strata[i]=255;
 auto hashView=[&](std::vector<uint8_t>& b){return MaterialSha256(b.data(),b.size());};
 auto desc=Value();Set(desc,"apiVersion",Num(2));Set(desc,"T_dataLayoutVersion",Num(2));Set(desc,"zOriginVersion",Num(1));
 Set(desc,"sessionId",Num(g.id));Set(desc,"surfaceEpoch",Num(g.egl.surfaceEpoch));Set(desc,"contextEpoch",Num(g.egl.contextEpoch));
 const uint8_t paletteBytes[6]={145,168,111,216,196,150};
 Set(desc,"assetGeneration",Num(1));Set(desc,"appearanceConfigRevision",Num(4));Set(desc,"geometryEpoch",Num(1));Set(desc,"renderSeed",Num(24));
 Set(desc,"grainWidth",Num(256));Set(desc,"grainHeight",Num(256));Set(desc,"strataWidth",Num(64));Set(desc,"strataHeight",Num(256));Set(desc,"layers",Num(3));
 Set(desc,"effectiveLambdaMaterial",Num(256./11.));Set(desc,"effectiveLambdaCanvasPx",Num(256./11.));Set(desc,"warpRangeCycles",Num(.25));
 const char* mkeys[]={"materialRecipeVersion","slopeKernelVersion","colorPipelineVersion","textureSamplingVersion","strataSamplingVersion","materialBundleId","paletteHash","assetGeneratorVersion","inputEncoding"};
 const char* mvals[]={"P1_ALBEDO_DELTA_V1","E2_GAUSS_COL6_R18_P1_V2","LIN_SRGB_REINHARD_V1","P1_TEXEL_CANVAS_PX_V1","CONTINUOUS_COLUMN_WARP_V1","T761_SYNTHETIC",MaterialSha256(paletteBytes,6).c_str(),"t761-v1","SRGB8_STRAIGHT_OPAQUE"};
 for(int i=0;i<9;i++)desc->fields[mkeys[i]]=Text(mvals[i]);
 desc->fields["grainBaseSrgb"]=Value();desc->fields["grainBaseSrgb"]->elements.push_back(List({145/255.,168/255.,111/255.}));desc->fields["grainBaseSrgb"]->elements.push_back(List({216/255.,196/255.,150/255.}));desc->fields["grainBaseSrgb"]->elements.push_back(List({181/255.,182/255.,131/255.}));
 auto bytesOf=[&](std::vector<uint8_t>& b){auto v=Value();v->typed=true;v->arrayType=napi_uint8_array;v->length=b.size();v->offset=16;v->buffer=Value();v->buffer->bytes.resize(b.size()+16);memcpy(v->buffer->bytes.data()+16,b.data(),b.size());return v;};
 auto grainV=bytesOf(grain),strataV=bytesOf(strata);
 desc->fields["grainHash"]=Text(hashView(grain).c_str());desc->fields["strataHash"]=Text(hashView(strata).c_str());
 // Binding 1 (rev 4) via the real bridge entry.
 auto binding=Value();Set(binding,"sessionId",Num(g.id));Set(binding,"appearanceConfigRevision",Num(4));binding->fields["configurationDigest"]=Text(std::string(64,'a').c_str());
 CHECK(Read(Call(BindMaterialConfiguration,{binding}),"code")==0);
 auto staged=Call(StageMaterialBundle,{desc,grainV,strataV});
 if(Read(staged,"code")!=0){std::printf("T761 stage FAILED code=%g errorStage=%s state=%s\n",Read(staged,"code"),ReadText(staged,"errorStage").c_str(),ReadText(staged,"state").c_str());return 1;}
 MaterialReceipt pumpState;do{auto r=Call(PumpMaterialUploads,{Num(65536)});pumpState.state=ReadText(r,"state");CHECK(Read(r,"code")==0);}while(pumpState.state!="READY");
 CHECK(g.material.Inspect().state=="READY");
 // --- Per-entry coverage point 1: bind-402 refusal on the SAME instance with READY pending ---
 auto conflictBind=Value();Set(conflictBind,"sessionId",Num(g.id));Set(conflictBind,"appearanceConfigRevision",Num(4));conflictBind->fields["configurationDigest"]=Text(std::string(64,'b').c_str()); // SAME rev, different digest -> 402 per T75 contract
 auto refused=Call(BindMaterialConfiguration,{conflictBind});
 CHECK(Read(refused,"code")==402);
 CHECK(g.material.Inspect().state=="READY");          // refusal keeps READY
 CHECK(g.material.Inspect().pendingBytes==0);          // pending fully uploaded
 // drawMaterialFrame still works on the untouched READY pending after refusal
 auto frame=Value();const char* ids[]={"sessionId","frameId","snapshotId","surfaceEpoch","fieldEpoch","geometryEpoch","continuityEpoch","lightConfigRevision"};
 for(auto key:ids)Set(frame,key,Num(1));
 Set(frame,"sessionId",Num(g.id));Set(frame,"surfaceEpoch",Num(g.egl.surfaceEpoch));Set(frame,"appearanceConfigRevision",Num(4));Set(frame,"frameTimeNs",Num(0));
 Set(frame,"surfaceWidthPx",Num(64));Set(frame,"surfaceHeightPx",Num(64));Set(frame,"boxXPx",Num(0));Set(frame,"boxYPx",Num(0));Set(frame,"boxWidthPx",Num(64));Set(frame,"boxHeightPx",Num(64));Set(frame,"capacity",Num(256));
 Set(frame,"T_dataLayoutVersion",Num(2));Set(frame,"zOriginVersion",Num(1));Set(frame,"apiVersion",Num(2));Set(frame,"assetGeneration",Num(1));Set(frame,"contextEpoch",Num(g.egl.contextEpoch));
 frame->fields["visualInverted"]=BooleanValue(false);frame->fields["diagnosticEnabled"]=BooleanValue(false);frame->fields["lightEnabled"]=BooleanValue(false);frame->fields["frameToken"]=Text("t761-token-1");
 const char* vkeys[]={"materialRecipeVersion","slopeKernelVersion","colorPipelineVersion","textureSamplingVersion","strataSamplingVersion"};
 const char* vvals[]={"P1_ALBEDO_DELTA_V1","E2_GAUSS_COL6_R18_P1_V2","LIN_SRGB_REINHARD_V1","P1_TEXEL_CANVAS_PX_V1","CONTINUOUS_COLUMN_WARP_V1"};
 for(int i=0;i<5;i++)frame->fields[vkeys[i]]=Text(vvals[i]);
 auto light=Value();const char* lk[]={"la","ld","lmin","lmax","lref","ellLightMaterial","lightX","lightY","lightZ","sigmaCandidate"};
 double lv[]={.85,.15,.8,1.12,1,24,-.55,-.55,.63,6};for(int i=0;i<10;i++)Set(light,lk[i],Num(lv[i]));
 auto material=Value();material->fields["paletteBaseSrgb"]=List({145/255.,168/255.,111/255.,216/255.,196/255.,150/255.});
 material->fields["grainBaseSrgb"]=List({145/255.,168/255.,111/255.,216/255.,196/255.,150/255.,181/255.,182/255.,131/255.});
 material->fields["grainOffsetCanvasPx"]=List({0,0,0});material->fields["bandMinYCanvasPx"]=List({0,0,0});material->fields["bandMaxYCanvasPx"]=List({64,64,64});
 material->fields["canvasBox"]=List({0,0,64,64});Set(material,"canvasToSurfaceScaleX",Num(1));Set(material,"canvasToSurfaceScaleY",Num(1));
 Set(material,"aB",Num(.75));Set(material,"aC",Num(.5));Set(material,"aC3",Num(.10));Set(material,"warpRangeCycles",Num(.25));
 Set(material,"effectiveLambdaMaterial",Num(256./11.));Set(material,"effectiveLambdaCanvasPx",Num(256./11.));
 for(auto key:{"grainEnabled","strataEnabled","sparkEnabled"})material->fields[key]=BooleanValue(true);
 auto view=Value();view->typed=true;view->arrayType=napi_float32_array;view->length=1536;view->offset=16;view->buffer=Value();view->buffer->bytes.resize(6144+16);
 auto* data=reinterpret_cast<float*>(view->buffer->bytes.data()+16);
 for(int c=0;c<128;c++){data[c*4]=120;data[c*4+1]=200;data[c*4+2]=40;data[c*4+3]=80;data[(128+c)*4+1]=.5;data[(256+c)*4+1]=1;}
 auto draw=[&](){return Call(DrawMaterialFrame,{frame,view,light,material});};
 auto d1=draw();
 if(Read(d1,"code")!=0||!d1->fields["swapSucceeded"]->boolean){std::printf("T761 draw1 FAILED code=%g errorStage=%s detail=%s\n",Read(d1,"code"),ReadText(d1,"errorStage").c_str(),ReadText(d1,"detail").c_str());return 1;}
 CHECK(g.material.Inspect().state=="ACTIVE"&&ReadText(d1,"frameToken")=="t761-token-1");
 std::puts("PASS T81 entry coverage: READY pending survives bind-402 refusal and still draws (same instance)");
 // --- Per-entry coverage point 2: activate->rebind->eligibility loss on same instance ---
 // New generation staged+ready while ACTIVE, then rebind to a new target: pending must lose activation eligibility.
 desc->fields["assetGeneration"]=Num(2);
 grain[100]=137;grainV=bytesOf(grain);desc->fields["grainHash"]=Text(hashView(grain).c_str());
 auto staged2=Call(StageMaterialBundle,{desc,grainV,strataV});CHECK(Read(staged2,"code")==0);
 do{auto r=Call(PumpMaterialUploads,{Num(65536)});pumpState.state=ReadText(r,"state");CHECK(Read(r,"code")==0);}while(pumpState.state!="READY");
 // rebind to revision 5 (new target) now succeeds (different revision => new binding)
 auto rebind=Value();Set(rebind,"sessionId",Num(g.id));Set(rebind,"appearanceConfigRevision",Num(5));rebind->fields["configurationDigest"]=Text(std::string(64,'b').c_str());
 CHECK(Read(Call(BindMaterialConfiguration,{rebind}),"code")==0);
 // old-generation activation must now be refused (bound target moved)
 auto activateOld=Call(ActivateMaterial,{Num(2),Text("t761-token-2")});
 if(Read(activateOld,"code")!=402){std::printf("T761 activateOld code=%g state=%s errorStage=%s pendingState=%s\n",Read(activateOld,"code"),ReadText(activateOld,"state").c_str(),ReadText(activateOld,"errorStage").c_str(),g.material.Inspect().state.c_str());return 1;}
 CHECK(g.material.Inspect().state=="READY");           // pending retained
 CHECK(g.material.ActiveDesc()!=nullptr&&g.material.ActiveDesc()->generation==1);
 std::puts("PASS T81 entry coverage: activate after rebind refused 402, READY pending retained (same instance)");
 // --- Per-entry coverage point 3: rebind back restores eligibility and draw works ---
 auto rebindBack=Value();Set(rebindBack,"sessionId",Num(g.id));Set(rebindBack,"appearanceConfigRevision",Num(4));rebindBack->fields["configurationDigest"]=Text(std::string(64,'a').c_str());
 CHECK(Read(Call(BindMaterialConfiguration,{rebindBack}),"code")==0);
 // binding id is monotone: the gen-2 pending still carries the OLD binding id, so re-binding back
 // to revision 4 does NOT silently re-eligibilize it (prevents replaying a stage across binding moves).
 CHECK(!g.material.CanActivate(2,g.egl.contextEpoch));
 auto activateStale=Call(ActivateMaterial,{Num(2),Text("t761-token-2")});
 CHECK(Read(activateStale,"code")==402);
 // re-stage under the restored binding as a NEW generation (generation reuse is refused: monotone
 // high-water identity), so the fresh READY generation 3 carries the stage-time binding.
 desc->fields["assetGeneration"]=Num(3);
 grain[100]=138;grainV=bytesOf(grain);desc->fields["grainHash"]=Text(hashView(grain).c_str());
 CHECK(Read(Call(StageMaterialBundle,{desc,grainV,strataV}),"code")==0);
 do{auto r=Call(PumpMaterialUploads,{Num(65536)});pumpState.state=ReadText(r,"state");CHECK(Read(r,"code")==0);}while(pumpState.state!="READY");
 CHECK(g.material.CanActivate(3,g.egl.contextEpoch));
 Set(frame,"frameId",Num(2));Set(frame,"assetGeneration",Num(3));
 auto d2=draw();CHECK(Read(d2,"code")==0&&d2->fields["swapSucceeded"]->boolean&&Read(d2,"assetGeneration")==3);
 std::puts("PASS T81 entry coverage: rebind back + re-stage restores eligibility, drawMaterialFrame consumes fresh generation");
 std::puts("PASS T81 all per-entry same-instance sequencing checks; host NAPI stub boundary, no Ark/device claims");
 return 0;
}
int main(){return RunT761Entries();}
