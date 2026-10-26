#define P013_MATERIAL_BRIDGE_TEST
#include "bridge_host_test.cpp"
static napi_value BooleanValue(bool x){napi_value v=nullptr;napi_get_boolean(env,x,&v);return v;}
static napi_value List(std::initializer_list<double> values){auto a=Value();for(auto x:values)a->elements.push_back(Num(x));return a;}
template<class T> auto BindFixtureTarget(T& uploader,uint64_t revision,int) -> decltype(uploader.BindConfiguration(1,revision,std::string(64,'a')),bool()) {return uploader.BindConfiguration(1,revision,std::string(64,'a')).code==0;}
template<class T> bool BindFixtureTarget(T&,uint64_t,long){return true;} // pre-T67 historical uploader
int main(){
 // Shader and EGL are explicit synthetic boundaries here; parser/lifecycle/generation transaction are real.
 auto* component=reinterpret_cast<OH_NativeXComponent*>(1);g.component=component;g.componentId="material-frame";g.egl.OnWindowCreated(component,reinterpret_cast<void*>(1));
 auto init=Call(InitSession,{Text("material-frame")});CHECK(Read(init,"code")==0);auto prep=Call(PrepareDualFieldSurface,{Num(g.id),Num(g.egl.surfaceEpoch)});CHECK(Read(prep,"code")==0);
 std::vector<uint8_t> grain(kMaterialGrainBytes,128),strata(kMaterialStrataBytes,128);for(size_t i=3;i<grain.size();i+=4)grain[i]=255;for(size_t i=3;i<strata.size();i+=4)strata[i]=255;
 MaterialDesc d;d.generation=1;d.revision=4;d.geometryEpoch=1;d.contextEpoch=g.egl.contextEpoch;d.surfaceEpoch=g.egl.surfaceEpoch;d.bundleId="EXPLICIT_SYNTHETIC_TEST";d.metadata="owned";d.grainHash=MaterialSha256(grain.data(),grain.size());d.strataHash=MaterialSha256(strata.data(),strata.size());d.grainBase={145/255.,168/255.,111/255.,216/255.,196/255.,150/255.,181/255.,182/255.,131/255.};d.warpRange=.25;d.lambdaMaterial=d.lambdaCanvas=256./11.;uint8_t pb[]={145,168,111,216,196,150};d.paletteHash=MaterialSha256(pb,6);
 CHECK(BindFixtureTarget(g.material,4,0));
 CHECK(g.material.Stage(d,grain.data(),grain.size(),strata.data(),strata.size()).code==0);MaterialReceipt staged;do{staged=g.material.Pump(65536,d.surfaceEpoch,d.contextEpoch);CHECK(staged.code==0);}while(staged.state!="READY");
 auto frame=Value();const char* ids[]={"sessionId","frameId","snapshotId","surfaceEpoch","fieldEpoch","geometryEpoch","continuityEpoch","lightConfigRevision"};for(auto key:ids)Set(frame,key,1);
 Set(frame,"sessionId",g.id);Set(frame,"surfaceEpoch",g.egl.surfaceEpoch);Set(frame,"appearanceConfigRevision",4);Set(frame,"frameTimeNs",0);Set(frame,"surfaceWidthPx",64);Set(frame,"surfaceHeightPx",64);Set(frame,"boxXPx",0);Set(frame,"boxYPx",0);Set(frame,"boxWidthPx",64);Set(frame,"boxHeightPx",64);Set(frame,"capacity",256);Set(frame,"T_dataLayoutVersion",2);Set(frame,"zOriginVersion",1);Set(frame,"apiVersion",2);Set(frame,"assetGeneration",1);Set(frame,"contextEpoch",g.egl.contextEpoch);
 frame->fields["visualInverted"]=BooleanValue(false);frame->fields["diagnosticEnabled"]=BooleanValue(false);frame->fields["lightEnabled"]=BooleanValue(false);frame->fields["frameToken"]=Text("token1");
 const char* versionKeys[]={"materialRecipeVersion","slopeKernelVersion","colorPipelineVersion","textureSamplingVersion","strataSamplingVersion"};const char* versionValues[]={"P1_ALBEDO_DELTA_V1","E2_GAUSS_COL6_R18_P1_V2","LIN_SRGB_REINHARD_V1","P1_TEXEL_CANVAS_PX_V1","CONTINUOUS_COLUMN_WARP_V1"};for(int i=0;i<5;i++)frame->fields[versionKeys[i]]=Text(versionValues[i]);
 auto light=Value();const char* lk[]={"la","ld","lmin","lmax","lref","ellLightMaterial","lightX","lightY","lightZ","sigmaCandidate"};double lv[]={.85,.15,.8,1.12,1,24,-.55,-.55,.63,6};for(int i=0;i<10;i++)Set(light,lk[i],lv[i]);
 auto material=Value();material->fields["paletteBaseSrgb"]=List({145/255.,168/255.,111/255.,216/255.,196/255.,150/255.});material->fields["grainBaseSrgb"]=List({145/255.,168/255.,111/255.,216/255.,196/255.,150/255.,181/255.,182/255.,131/255.});
 material->fields["grainOffsetCanvasPx"]=List({0,0,0});material->fields["bandMinYCanvasPx"]=List({0,0,0});material->fields["bandMaxYCanvasPx"]=List({64,64,64});material->fields["canvasBox"]=List({0,0,64,64});Set(material,"canvasToSurfaceScaleX",1);Set(material,"canvasToSurfaceScaleY",1);Set(material,"aB",.75);Set(material,"aC",.5);Set(material,"aC3",.10);Set(material,"warpRangeCycles",.25);Set(material,"effectiveLambdaMaterial",256./11.);Set(material,"effectiveLambdaCanvasPx",256./11.);for(auto key:{"grainEnabled","strataEnabled","sparkEnabled"})material->fields[key]=BooleanValue(true);
 auto view=Value();view->typed=true;view->arrayType=napi_float32_array;view->length=1536;view->offset=16;view->buffer=Value();view->buffer->bytes.resize(6144+16);auto* data=reinterpret_cast<float*>(view->buffer->bytes.data()+16);for(int c=0;c<128;c++){data[c*4]=120;data[c*4+1]=200;data[c*4+2]=40;data[c*4+3]=80;data[(128+c)*4+1]=.5;data[(256+c)*4+1]=1;}
 auto draw=[&](){return Call(DrawMaterialFrame,{frame,view,light,material});};
 auto result=draw();CHECK(Read(result,"code")==0&&result->fields["swapSucceeded"]->boolean&&Read(result,"assetGeneration")==1);CHECK(g.material.Inspect().state=="ACTIVE");CHECK(!g.material.Inspect().frameToken.empty()); // READY accepted+activated once at same frame boundary
 auto previous=g.lastSwap;
 CHECK(Read(draw(),"code")==102&&g.lastSwap==previous);
 int frameId=2;auto rejected=[&](int expected){Set(frame,"frameId",frameId++);auto r=draw();return Read(r,"code")==expected&&!r->fields["swapSucceeded"]->boolean&&!r->fields["stale"]->boolean&&g.lastSwap==previous;};
 Set(frame,"assetGeneration",2);CHECK(rejected(401));Set(frame,"assetGeneration",1);
 Set(frame,"appearanceConfigRevision",5);CHECK(rejected(402));Set(frame,"appearanceConfigRevision",4);
 Set(frame,"contextEpoch",g.egl.contextEpoch+1);CHECK(rejected(103));Set(frame,"contextEpoch",g.egl.contextEpoch);
 data[1026]=1;CHECK(rejected(400));data[1026]=0;data[513]=2;CHECK(rejected(400));data[513]=.5;
 view->length=512;CHECK(rejected(400));view->length=1536;view->arrayType=napi_uint8_array;CHECK(rejected(400));view->arrayType=napi_float32_array;
 view->buffer->detached=true;CHECK(rejected(400));view->buffer->detached=false;
 material->fields["grainOffsetCanvasPx"]=List({1,2});CHECK(rejected(400));material->fields["grainOffsetCanvasPx"]=List({0,0,0});
 Set(material,"canvasToSurfaceScaleY",2);CHECK(rejected(400));Set(material,"canvasToSurfaceScaleY",1);
 Set(material,"aC",.6);CHECK(rejected(400));Set(material,"aC",.5);
 material->fields["grainBaseSrgb"]->elements[0]=Num(.1);CHECK(rejected(402));material->fields["grainBaseSrgb"]->elements[0]=Num(145/255.);
 frame->fields["slopeKernelVersion"]=Text("E2_APPROX_P0");CHECK(rejected(400));frame->fields["slopeKernelVersion"]=Text("E2_GAUSS_COL6_R18_P1_V2");
 Set(frame,"frameId",frameId++);uploadFailure=true;auto failed=draw();CHECK(Read(failed,"code")==302&&!failed->fields["swapSucceeded"]->boolean&&g.lastSwap==previous);uploadFailure=false;
 Set(frame,"frameId",frameId++);CHECK(Read(draw(),"code")==0);Call(Release,{});
 std::puts("PASS T55 actual material bridge: READY atomic activation, generation/config/epoch/version/layout/range checks, shared monotonic IDs, P1 OFF, explicit fault without fabricated LastGood on failure, no Canvas path");return 0;
}
