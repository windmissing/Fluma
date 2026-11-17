#pragma once
#include "../cold_protocol.h"
#include <array>
#include <cstring>
namespace grs::input537 {
struct Row {diagnostic492::Scope scope{};uint64_t ns=0;std::array<char,440> raw{};uint16_t bytes=0,code=0;uint32_t serial=0;};
static_assert(sizeof(Row)==512,"T530 input-stage slot must remain 512 bytes");
struct Ledger {
 std::array<Row,32> rows{};std::array<uint64_t,9> counts{};
 uint64_t seen=0,dropped=0,retiredScopes=0;uint32_t extra=9;diagnostic492::Scope scope{};bool scoped=false;
 void Add(uint16_t code,const diagnostic492::Point& point,const char* raw,size_t bytes){
  if(!scoped||scope.epoch!=point.scope.epoch||scope.run!=point.scope.run||scope.session!=point.scope.session){if(scoped&&retiredScopes!=UINT64_MAX)++retiredScopes;rows={};counts={};seen=dropped=0;extra=9;scope=point.scope;scoped=true;}
  if(code<1||code>9||bytes>=440||seen==UINT64_MAX){if(dropped!=UINT64_MAX)++dropped;return;}
  if(counts[code-1]!=UINT64_MAX)++counts[code-1];++seen;
  // Protect the first page Down/Move/Up/Cancel, controller predicate and
  // native action/Offer even if a Move burst exhausts the ordinary tail.
  uint32_t slot=code-1;if(rows[slot].code){if(extra==rows.size()){++dropped;return;}slot=extra++;}
  auto& row=rows[slot];row={};row.scope=point.scope;row.ns=point.ns;row.code=code;row.serial=uint32_t(std::min<uint64_t>(seen,UINT32_MAX));row.bytes=uint16_t(bytes);std::memcpy(row.raw.data(),raw,bytes);
 }
 void Json(cold503::Writer& w)const{
  w.Raw("{\"revision\":\"T537_INPUT_STAGE_V1\",\"slotBytes\":512,\"slots\":32,\"scope\":");cold503::ControlJson(w,scope);w.Raw(",\"retiredScopes\":");w.U64(retiredScopes);w.Raw(",\"seen\":");w.U64(seen);w.Raw(",\"dropped\":");w.U64(dropped);w.Raw(",\"counts\":[");for(size_t i=0;i<counts.size();++i){if(i)w.Raw(",");w.U64(counts[i]);}w.Raw("],\"rows\":[");bool comma=false;
  for(const auto& row:rows)if(row.code){if(comma)w.Raw(",");comma=true;w.Raw("{\"serial\":");w.Number(row.serial);w.Raw(",\"code\":");w.Number(row.code);w.Raw(",\"nativeNs\":");w.U64(row.ns);w.Raw(",\"scope\":");cold503::ControlJson(w,row.scope);w.Raw(",\"raw\":");w.String(row.raw.data(),row.bytes);w.Raw("}");}w.Raw("]}");
 }
 void Packet(const diagnostic492::Point& point,const std::array<uint32_t,32>& words,const char* reply){
  std::array<char,440> bytes{};cold503::Writer writer(bytes.data(),bytes.size());writer.Raw("{\"origin\":\"NATIVE_OFFER\",\"raw128B_LE\":");writer.Hex(words.data(),128);writer.Raw(",\"reply\":");writer.String(reply,std::strlen(reply));writer.Raw("}");if(writer.Good())Add(7,point,writer.Data(),writer.Size());else ++dropped;
 }
};
struct Pixel {
 uint64_t epoch=0,created=0,until=0,swapBegin=0,swapEnd=0;uint32_t width=0,height=0;
 bool fboRead=false,defaultRead=false,swap=false,failed=false;uint32_t gl=0,egl=0,program=0,fbo=0,texture=0;
 std::array<int32_t,4> viewport{},scissor{};bool blend=false,scissorEnabled=false;
 std::array<uint8_t,12> fboPixels{},defaultPixels{};std::array<int32_t,6> roi{};
 void Json(cold503::Writer& w,uint64_t now,bool current)const{
  w.Raw("{\"revision\":\"T537_PIXEL_HOLD_V1\",\"state\":");const char* state=failed?"FAULT":!current?"STALE_SCOPE":!created?"WAIT_FRAME":now>until?"HOLD_EXPIRED":gl||egl||!swap?"INCOMPLETE":"DIAGNOSTIC_HOLD";w.String(state,std::strlen(state));
  w.Raw(",\"evidenceOrigin\":\"DIAGNOSTIC_NOT_PHYSICS\",\"physicsValidated\":false,\"acceptedCommitIncrement\":0,\"token\":\"T537_DIAGNOSTIC_NOT_PHYSICS|");w.Number(epoch);w.Raw("|");w.Number(created);w.Raw("\",\"epoch\":");w.U64(epoch);w.Raw(",\"createdNs\":");w.U64(created);w.Raw(",\"holdUntilNs\":");w.U64(until);w.Raw(",\"sampleNs\":");w.U64(now);w.Raw(",\"swapBeginNs\":");w.U64(swapBegin);w.Raw(",\"swapEndNs\":");w.U64(swapEnd);w.Raw(",\"width\":");w.Number(width);w.Raw(",\"height\":");w.Number(height);w.Raw(",\"rawGl\":");w.Number(gl);w.Raw(",\"rawEgl\":");w.Number(egl);w.Raw(",\"fbo\":");w.Number(fbo);w.Raw(",\"program\":");w.Number(program);w.Raw(",\"sourceTexture\":");w.Number(texture);w.Raw(",\"samplerUnit\":1,\"blend\":");w.Bool(blend);w.Raw(",\"scissorEnabled\":");w.Bool(scissorEnabled);
  const auto array=[&](const char* name,const auto& values){w.Raw(",\"");w.Raw(name);w.Raw("\":[");for(size_t i=0;i<values.size();++i){if(i)w.Raw(",");w.Number(values[i]);}w.Raw("]");};array("viewport",viewport);array("scissor",scissor);array("roiBottomLeftPx",roi);array("fboRgba8",fboPixels);array("defaultRgba8",defaultPixels);
  w.Raw(",\"fboRead\":");w.Bool(fboRead);w.Raw(",\"defaultRead\":");w.Bool(defaultRead);w.Raw(",\"swapSucceeded\":");w.Bool(swap);w.Raw(",\"systemPng\":\"NOT_MEASURED_EXTERNAL_CAPTURE_REQUIRED\"}");
 }
};
}
