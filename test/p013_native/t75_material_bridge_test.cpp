#define P013_MATERIAL_BRIDGE_TEST
#include "bridge_host_test.cpp"
#define main RunT75Core
#include "t75_material_core_test.cpp"
#undef main
static napi_value Array(std::initializer_list<napi_value> elements){auto v=Value();v->elements=elements;return v;}
static napi_value Bytes(size_t n){auto v=Value();v->typed=true;v->arrayType=napi_uint8_array;v->length=n;v->offset=16;v->buffer=Value();v->buffer->bytes.resize(n+16);for(size_t i=0;i<n;i++)v->buffer->bytes[i+16]=(i%4==3)?255:128;return v;}
int main(){
 CHECK(RunT75Core()==0);
 CHECK(RunP013BridgeBaseline()==0);
 auto init=Call(p013::InitSession,{Text("test")});CHECK(Read(init,"code")==0);
 auto stalePrepare=Call(p013::PrepareDualFieldSurface,{Num(Read(init,"sessionId")),Num(p013::g.egl.surfaceEpoch+1)});CHECK(Read(stalePrepare,"code")==103);CHECK(!p013::g.egl.composition);
 auto prep=Call(p013::PrepareDualFieldSurface,{Num(Read(init,"sessionId")),Num(p013::g.egl.surfaceEpoch)});CHECK(Read(prep,"code")==0);
 CHECK(p013::g.egl.composition);
 auto grain=Bytes(786432),strata=Bytes(196608),desc=Value();
 for(auto k:{"apiVersion","T_dataLayoutVersion"})Set(desc,k,2);
 Set(desc,"zOriginVersion",1);Set(desc,"sessionId",Read(init,"sessionId"));Set(desc,"surfaceEpoch",p013::g.egl.surfaceEpoch);Set(desc,"contextEpoch",p013::g.egl.contextEpoch);
 Set(desc,"assetGeneration",1);Set(desc,"appearanceConfigRevision",1);Set(desc,"geometryEpoch",1);Set(desc,"renderSeed",24);
 Set(desc,"grainWidth",256);Set(desc,"grainHeight",256);Set(desc,"strataWidth",64);Set(desc,"strataHeight",256);Set(desc,"layers",3);
 Set(desc,"effectiveLambdaMaterial",4);Set(desc,"effectiveLambdaCanvasPx",8);Set(desc,"warpRangeCycles",1);
 const char* keys[]={"materialRecipeVersion","slopeKernelVersion","colorPipelineVersion","textureSamplingVersion","strataSamplingVersion","materialBundleId","paletteHash","assetGeneratorVersion","inputEncoding"};
 const char* vals[]={"P1_ALBEDO_DELTA_V1","E2_GAUSS_COL6_R18_P1_V2","LIN_SRGB_REINHARD_V1","P1_TEXEL_CANVAS_PX_V1","CONTINUOUS_COLUMN_WARP_V1","SYNTHETIC","synthetic-palette","synthetic-v1","SRGB8_STRAIGHT_OPAQUE"};
 for(int i=0;i<9;i++)desc->fields[keys[i]]=Text(vals[i]);
 desc->fields["grainBaseSrgb"]=Array({Array({Num(.5),Num(.5),Num(.5)}),Array({Num(.5),Num(.5),Num(.5)}),Array({Num(.5),Num(.5),Num(.5)})});
 auto hash=[&](napi_value view){return p013::MaterialSha256(view->buffer->bytes.data()+view->offset,view->length);};
 desc->fields["grainHash"]=Text(hash(grain).c_str());desc->fields["strataHash"]=Text(hash(strata).c_str());
 auto binding=Value();Set(binding,"sessionId",Read(init,"sessionId"));Set(binding,"appearanceConfigRevision",1);binding->fields["configurationDigest"]=Text(std::string(64,'a').c_str());
 CHECK(Read(Call(p013::BindMaterialConfiguration,{binding}),"code")==0);
 auto stage=[&](){return Call(p013::StageMaterialBundle,{desc,grain,strata});};
 CHECK(Read(stage(),"code")==0);
 desc->fields["assetGenerationParameters"]=Text(std::string(300,'p').c_str());CHECK(stage()->fields["errorStage"]->text=="identity");desc->fields.erase("assetGenerationParameters");
 const auto beforePumps=p013::g.material.Inspect().pumpCount;
 for(auto bad:{Num(0),Num(-1),Num(1.5),Num(65537),Text("1024"),Value(napi_boolean)}){auto r=Call(p013::PumpMaterialUploads,{bad});CHECK(Read(r,"code")==400&&r->fields["errorStage"]->text=="byteBudget"&&Read(r,"materialUploadBytes")==0);}
 CHECK(Read(Call(p013::PumpMaterialUploads,{}),"code")==400);
 CHECK(Read(Call(p013::PumpMaterialUploads,{Num(1024),Num(1)}),"code")==400);
 CHECK(p013::g.material.Inspect().pumpCount==beforePumps);
 Set(desc,"appearanceConfigRevision",2);CHECK(Read(stage(),"code")==402);Set(desc,"appearanceConfigRevision",1);
 auto oldId=desc->fields["materialBundleId"];desc->fields["materialBundleId"]=Text("conflict");CHECK(stage()->fields["errorStage"]->text=="identity");desc->fields["materialBundleId"]=oldId;
 auto saved=desc->fields["paletteHash"];desc->fields.erase("paletteHash");CHECK(Read(stage(),"code")==400);desc->fields["paletteHash"]=saved;
 std::puts("PASS TC-P013-G2-A01 metadata required");
 saved=desc->fields["materialRecipeVersion"];desc->fields["materialRecipeVersion"]=Text("unknown");CHECK(Read(stage(),"code")==400);desc->fields.erase("materialRecipeVersion");CHECK(Read(stage(),"code")==400);desc->fields["materialRecipeVersion"]=saved;
 std::puts("PASS TC-P013-G2-A02 A03 seven versions strict");
 grain->length--;CHECK(Read(stage(),"code")==400);grain->length++;
 grain->arrayType=napi_float32_array;CHECK(Read(stage(),"code")==400);grain->arrayType=napi_uint8_array;
 grain->offset=20;CHECK(Read(stage(),"code")==400);grain->offset=16;grain->buffer->detached=true;CHECK(Read(stage(),"code")==400);grain->buffer->detached=false;
 std::puts("PASS TC-P013-G2-A06 A07 exact byte range/type/offset/detach");
 grain->buffer->bytes[16]=129;CHECK(Read(stage(),"code")==400);grain->buffer->bytes[16]=128;
 grain->buffer->bytes[19]=0;desc->fields["grainHash"]=Text(hash(grain).c_str());auto badAlpha=stage();CHECK(Read(badAlpha,"code")==400&&badAlpha->fields["errorStage"]->text=="alpha");grain->buffer->bytes[19]=255;desc->fields["grainHash"]=Text(hash(grain).c_str());
 std::puts("PASS TC-P013-G2-A08 A09 actual SHA256 and alpha");
 Set(desc,"contextEpoch",p013::g.egl.contextEpoch+1);CHECK(Read(stage(),"code")==400);Set(desc,"contextEpoch",p013::g.egl.contextEpoch);
 std::puts("PASS TC-P013-G2-A10 context epoch strict");
 CHECK(Read(stage(),"code")==0);auto off=grain->buffer->bytes[16];grain->buffer->bytes[16]=7;
 for(int i=0;i<15;i++){auto r=Call(p013::PumpMaterialUploads,{Num(65536)});CHECK(Read(r,"code")==0&&Read(r,"uploadedBytes")<=65536);}
 grain->buffer->bytes[16]=off;
 auto ready=Call(p013::CanActivateMaterial,{Num(1),Num(p013::g.egl.contextEpoch)});CHECK(ready->boolean);
 auto active=Call(p013::ActivateMaterial,{Num(1),Text("frame-token-1")});CHECK(Read(active,"activeGeneration")==1&&active->fields["state"]->text=="ACTIVE");
 Call(p013::Release,{});
 std::puts("PASS G2 actual five-entry bridge; synthetic NAPI/GPU boundary only");return 0;
}
