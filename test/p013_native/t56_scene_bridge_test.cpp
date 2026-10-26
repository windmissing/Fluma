#define P013_MATERIAL_BRIDGE_TEST
#define P013_SCENE_BRIDGE_TEST
#include "bridge_host_test.cpp"
namespace p013 {
static int sceneLayers=0;static bool sceneLayerFailure=false;
Status SceneGlGpu::Begin(const ScenePacket&){return Status::OK;}
Status SceneGlGpu::Layer(int layer,const ScenePacket&,const SceneConfiguration&,const SceneLease*){sceneLayers++;return sceneLayerFailure&&layer==4?Status::GL_DRAW_FAILED:Status::OK;}
Status SceneGlGpu::Present(bool){return Status::OK;}
void FullFrameTargets::Reset(bool){textures[0]=textures[1]=fbos[0]=fbos[1]=0;width=height=0;}
void SceneLayers::Reset(bool){program=present=vao=data=legacy=0;}
}
static napi_value View(napi_typedarray_type type,size_t count,size_t stride){auto v=Value();v->typed=true;v->arrayType=type;v->length=type==napi_float64_array?count*2:count; /* historical host stub lengths use 4-byte units; native receives exact ByteLength */v->offset=16;v->buffer=Value();v->buffer->bytes.resize(count*stride+16);return v;}
int main(){
 CHECK(RunP013BridgeBaseline()==0);
 auto init=Call(p013::InitSession,{Text("test")});CHECK(Read(init,"code")==0);CHECK(Read(Call(p013::PrepareDualFieldSurface,{Num(Read(init,"sessionId")),Num(p013::g.egl.surfaceEpoch)}),"code")==0);
 auto values=View(napi_float64_array,51,8);auto* v=reinterpret_cast<double*>(values->buffer->bytes.data()+16);v[1]=3;v[3]=24;v[4]=v[5]=1;v[8]=v[9]=64;v[10]=v[11]=v[12]=v[19]=1;v[13]=v[14]=24;v[15]=.25;double light[]={.85,.15,.8,1.12,1,24,-.55,-.55,.63,6};for(int i=0;i<10;i++)v[35+i]=light[i];v[50]=20261020;
 auto binding=Value();Set(binding,"sessionId",Read(init,"sessionId"));Set(binding,"appearanceConfigRevision",1);binding->fields["configurationDigest"]=Text(p013::MaterialSha256(reinterpret_cast<uint8_t*>(v),51*8).c_str());auto bound=Call(p013::BindMaterialConfiguration,{binding});CHECK(Read(bound,"code")==0);
 auto reg=[&](){return Call(p013::RegisterSceneConfiguration,{bound,values});};CHECK(Read(reg(),"code")==0);
 values->offset=17;CHECK(Read(reg(),"code")==402);values->offset=16;v[12]=.9;CHECK(Read(reg(),"code")==400);v[12]=1;values->buffer->detached=true;CHECK(Read(reg(),"code")==402);values->buffer->detached=false;
 auto frame=Value();for(const char* key:{"sessionId","frameId","snapshotId","surfaceEpoch","fieldEpoch","geometryEpoch","continuityEpoch","appearanceConfigRevision","lightConfigRevision","frameTimeNs","surfaceWidthPx","surfaceHeightPx","boxXPx","boxYPx","boxWidthPx","boxHeightPx","capacity","T_dataLayoutVersion","zOriginVersion"})Set(frame,key,1);
 Set(frame,"sessionId",Read(init,"sessionId"));Set(frame,"surfaceEpoch",p013::g.egl.surfaceEpoch);Set(frame,"surfaceWidthPx",64);Set(frame,"surfaceHeightPx",64);Set(frame,"boxXPx",0);Set(frame,"boxYPx",0);Set(frame,"boxWidthPx",64);Set(frame,"boxHeightPx",64);Set(frame,"capacity",192);Set(frame,"T_dataLayoutVersion",2);Set(frame,"contextEpoch",p013::g.egl.contextEpoch);Set(frame,"stateEpoch",1);Set(frame,"physicsCommitId",1);Set(frame,"requestedGeneration",0);Set(frame,"particleCount",0);Set(frame,"particleCapacity",1);
 frame->fields["visualInverted"]=Value(napi_boolean);frame->fields["diagnosticEnabled"]=Value(napi_boolean);frame->fields["particleLayerEnabled"]=Value(napi_boolean);
 for(auto pair:{std::pair<const char*,const char*>{"scenePacketRevision","P013_SCENE_PACKET_V1"},{"renderContractRevision","P013_NEWPIPE_ONLY_V1"},{"slopeKernelVersion","E2_GAUSS_COL6_R18_P1_V2"},{"frameToken","scene1"},{"runId","run"},{"ledgerCommitKey",""},{"routeId","column-field"},{"mediumMode","WET_COLUMN_V1"},{"sourceFailure",""}})frame->fields[pair.first]=Text(pair.second);
 auto data=View(napi_float32_array,2048,4),xy=View(napi_float32_array,2,4),meta=View(napi_uint32_array,2,4),motion=View(napi_float64_array,10,8);
 auto draw=[&](){return Call(p013::DrawNewPipelineFrame,{frame,data,xy,meta,motion});};auto r=draw();CHECK(Read(r,"code")==0&&r->fields["currentFrameSucceeded"]->boolean&&p013::sceneLayers==7);
 CHECK(Read(draw(),"code")==102);Set(frame,"frameId",2);data->buffer->detached=true;r=draw();CHECK(Read(r,"code")==100&&!r->fields["currentFrameSucceeded"]->boolean);data->buffer->detached=false;
 frame->fields["sourceFailure"]=Text("INVALID_PARTICLE");r=draw();CHECK(r->fields["presentationState"]->text=="STALE"&&r->fields["staleSwapSucceeded"]->boolean&&!r->fields["requestedSwapSucceeded"]->boolean&&r->fields["lastGoodFrameToken"]->text=="scene1");frame->fields["sourceFailure"]=Text("");
 Set(frame,"frameId",3);p013::sceneLayerFailure=true;r=draw();CHECK(r->fields["firstFailureStage"]->text=="HIGHLIGHT"&&r->fields["presentationState"]->text=="STALE");p013::sceneLayerFailure=false;
 Set(frame,"frameId",4);double wrong=0;std::thread thread([&]{wrong=Read(draw(),"code");});thread.join();CHECK(wrong==104);CHECK(Read(draw(),"code")==0);
 auto* floats=reinterpret_cast<float*>(data->buffer->bytes.data()+16);Set(frame,"frameId",5);floats[0]=std::numeric_limits<float>::quiet_NaN();CHECK(Read(draw(),"code")==100);floats[0]=0;
 CHECK(Read(Call(p013::Release,{}),"code")==0);CHECK(!p013::g.scene.Desired()&&!p013::g.scene.HasLastGood());
 std::puts("PASS T56 actual scene bridge: Float64 config hash/binding/range/detach, owned data, full frame source, stale token, source failure replay, pass failure, wrong owner, NaN, release; synthetic GPU boundary");return 0;
}

\n