#pragma once
#include "cold_protocol.h"
namespace grs {namespace cold503 {
static_assert(sizeof(diagnostic492::Capture)<=1404072,"T503 capture limit; revise contract before enlarging");
inline void PointJson(Writer& w,const diagnostic492::Point& p){
 w.Raw("{\"stage\":");w.String(p.stage);w.Raw(",\"clock\":\"STEADY_NS\",\"ns\":");w.U64(p.ns);w.Raw(",\"tid\":");w.U64(p.tid);w.Raw(",\"root\":");w.U64(p.root);
 w.Raw(",\"a\":");w.U64(p.a);w.Raw(",\"b\":");w.U64(p.b);w.Raw(",\"c\":");w.U64(p.c);w.Raw(",\"callStart\":");w.U64(p.callStart);w.Raw(",\"callEnd\":");w.U64(p.callEnd);
 w.Raw(",\"code\":");w.Number(p.code);w.Raw(",\"rawGl\":");w.Number(p.gl);w.Raw(",\"rawEgl\":");w.Number(p.egl);w.Raw(",\"scope\":");ControlJson(w,p.scope);w.Raw("}");
}
inline void RootJson(Writer& w,const diagnostic492::Root& r){
 w.Raw("{\"nonce\":");w.U64(r.nonce);w.Raw(",\"started\":");w.U64(r.started);w.Raw(",\"deadline\":");w.U64(r.deadline);w.Raw(",\"kind\":");w.Number(r.kind);w.Raw(",\"plan\":");w.U64(r.plan);w.Raw(",\"scope\":");ControlJson(w,r.scope);
 w.Raw(",\"input\":[");w.Number(r.input.lo);w.Raw(",");w.Number(r.input.hi);w.Raw("],\"identityBytes\":[");const auto* b=reinterpret_cast<const uint8_t*>(&r.identity);for(size_t i=0;i<sizeof(r.identity);++i){if(i)w.Raw(",");w.Number(b[i]);}w.Raw("]}");
}
inline void CauseJson(Writer& w,const diagnostic492::Cause& c){if(!c.set){w.Raw("null");return;}w.Raw("{\"reason\":");w.String(c.reason);w.Raw(",\"point\":");PointJson(w,c.point);w.Raw(",\"latePulses\":");w.U64(c.latePulses);w.Raw("}");}
struct FirstGroup {diagnostic492::Cause cause{};diagnostic492::Root fault{};diagnostic492::Scope scope{};uint64_t dropped=0,quantumDropped=0,tailDropped=0;uint32_t count=0,quantumCount=0,tailCount=0;};
struct SmallDiagnostic {
 Scope scope{};std::array<FirstGroup,4> groups{};uint32_t groupCount=0,stageCount=0,actionCount=0,packetCount=0,resetCount=0;
 uint64_t scopeDropped=0,unknownStageCount=0,actionDropped=0,packetDropped=0,resetDropped=0;
 void Copy(const diagnostic492::Capture& c,const Scope& s){scope=s;groupCount=c.groupCount;stageCount=c.stageCount;actionCount=c.actionCount;packetCount=c.packetCount;resetCount=c.resetCount;scopeDropped=c.scopeDropped;unknownStageCount=c.unknownStageCount;actionDropped=c.actionDropped;packetDropped=c.packetDropped;resetDropped=c.resetDropped;for(size_t i=0;i<groups.size();++i){const auto& g=c.groups[i];groups[i]={g.cause,g.faultRoot,g.scope,g.dropped,g.quantumDropped,g.tailDropped,g.count,g.quantumCount,g.tailCount};}}
};
// Conservative bound: four maximum escaped reasons/points/identity byte lists,
// full U64 widths, scope strings (2*128*6), and all fixed counters.
inline constexpr size_t FallbackWorst=4*3000+2*128*6+2000;
static_assert(FallbackWorst<=PageBytes,"T503 fallback must fit 16KiB");
inline void CountsJson(Writer& w,const SmallDiagnostic& s){
 w.Raw("\"scopeDropped\":");w.Number(s.scopeDropped);w.Raw(",\"unknownStageCount\":");w.Number(s.unknownStageCount);w.Raw(",\"actionDropped\":");w.Number(s.actionDropped);w.Raw(",\"packetDropped\":");w.Number(s.packetDropped);w.Raw(",\"resetDropped\":");w.Number(s.resetDropped);
 w.Raw(",\"groupCount\":");w.Number(s.groupCount);w.Raw(",\"stageCount\":");w.Number(s.stageCount);w.Raw(",\"actionCount\":");w.Number(s.actionCount);w.Raw(",\"packetCount\":");w.Number(s.packetCount);w.Raw(",\"resetCount\":");w.Number(s.resetCount);
}
inline void DiagnosticFallback(Writer& w,const SmallDiagnostic& s,const char* why,size_t limit,bool stale=false){
 w.Reset();w.Raw("{\"revision\":\"T503_EXECUTION_DIAGNOSTICS_V1\",\"state\":\"INCOMPLETE\",\"dataComplete\":false,\"reason\":");w.String(why,std::strlen(why));w.Raw(",\"limitUtf8Bytes\":");w.Number(limit);w.Raw(",\"scopeStale\":");w.Bool(stale);w.Raw(",\"scope\":");ScopeJson(w,s.scope);w.Raw(",");CountsJson(w,s);w.Raw(",\"groups\":[");
 for(size_t i=0;i<std::min<size_t>(s.groupCount,4);++i){if(i)w.Raw(",");const auto& g=s.groups[i];w.Raw("{\"scope\":");ControlJson(w,g.scope);w.Raw(",\"firstCause\":");CauseJson(w,g.cause);w.Raw(",\"faultRoot\":");RootJson(w,g.fault);w.Raw(",\"dropped\":");w.Number(g.dropped);w.Raw(",\"quantumDropped\":");w.Number(g.quantumDropped);w.Raw(",\"tailDropped\":");w.Number(g.tailDropped);w.Raw(",\"count\":");w.Number(g.count);w.Raw(",\"quantumCount\":");w.Number(g.quantumCount);w.Raw(",\"tailCount\":");w.Number(g.tailCount);w.Raw("}");}w.Raw("]}");
}
inline void DiagnosticJson(Writer& w,const diagnostic492::Capture& c,const SmallDiagnostic& summary,size_t limit,bool stale){
 w.Raw("{\"revision\":\"T503_EXECUTION_DIAGNOSTICS_V1\",\"payloadRevision\":\"T492_EXECUTION_DIAGNOSTICS_V1\",\"state\":\"COMPLETE\",\"dataComplete\":true,\"scopeStale\":");w.Bool(stale);w.Raw(",\"limitUtf8Bytes\":");w.Number(limit);w.Raw(",\"scope\":");ScopeJson(w,summary.scope);w.Raw(",");CountsJson(w,summary);w.Raw(",\"stages\":[");
 for(size_t i=0;i<std::min<size_t>(c.stageCount,c.stages.size());++i){if(i)w.Raw(",");const auto& s=c.stages[i];w.Raw("{\"count\":");w.U64(s.count);w.Raw(",\"enters\":");w.U64(s.enters);w.Raw(",\"exits\":");w.U64(s.exits);w.Raw(",\"first\":");PointJson(w,s.first);w.Raw(",\"last\":");PointJson(w,s.last);w.Raw(",\"firstError\":");if(s.error)PointJson(w,s.firstError);else w.Raw("null");w.Raw("}");}
 w.Raw("],\"groups\":[");
 for(size_t i=0;i<std::min<size_t>(c.groupCount,c.groups.size());++i){if(i)w.Raw(",");const auto& g=c.groups[i];w.Raw("{\"scope\":");ControlJson(w,g.scope);w.Raw(",\"firstCause\":");CauseJson(w,g.cause);w.Raw(",\"root\":");RootJson(w,g.root);w.Raw(",\"faultRoot\":");RootJson(w,g.faultRoot);w.Raw(",\"dropped\":");w.Number(g.dropped);w.Raw(",\"quantumDropped\":");w.Number(g.quantumDropped);w.Raw(",\"tailDropped\":");w.Number(g.tailDropped);w.Raw(",\"firstQuantum\":");RootJson(w,g.quantum);
  const char* names[]={"events","firstQuantumEvents","tail"};const diagnostic492::Point* points[]={g.first.data(),g.firstQuantum.data(),g.tail.data()};const size_t count[]={std::min<size_t>(g.count,g.first.size()),std::min<size_t>(g.quantumCount,g.firstQuantum.size()),std::min<size_t>(g.tailCount,g.tail.size())};
  for(unsigned p=0;p<3;++p){w.Raw(",");w.String(names[p],std::strlen(names[p]));w.Raw(":[");for(size_t j=0;j<count[p];++j){if(j)w.Raw(",");PointJson(w,points[p][j]);}w.Raw("]");}w.Raw("}");
 }
 w.Raw("],\"actions\":[");for(size_t i=0;i<std::min<size_t>(c.actionCount,c.actions.size());++i){if(i)w.Raw(",");w.Raw("{\"native\":");PointJson(w,c.actions[i].point);w.Raw(",\"rawHex\":");size_t n=0;while(n<c.actions[i].raw.size()&&c.actions[i].raw[n])++n;w.Hex(c.actions[i].raw.data(),n);w.Raw("}");}
 w.Raw("],\"packets\":[");for(size_t i=0;i<std::min<size_t>(c.packetCount,c.packets.size());++i){if(i)w.Raw(",");const auto& p=c.packets[i];w.Raw("{\"native\":");PointJson(w,p.point);w.Raw(",\"request\":");w.Number(p.words[6]);w.Raw(",\"hashRevision\":\"FNV1A32_LE_128B\",\"hash\":");const uint8_t bytes[]={uint8_t(p.hash>>24),uint8_t(p.hash>>16),uint8_t(p.hash>>8),uint8_t(p.hash)};w.Hex(bytes,4);w.Raw(",\"reply\":");w.String(p.reply);p011gpu::Sha256 hash;for(auto word:p.words){const uint8_t b[]={uint8_t(word),uint8_t(word>>8),uint8_t(word>>16),uint8_t(word>>24)};hash.update(b,4);}const auto sha=hash.finish();w.Raw(",\"commandSHA256\":");w.Hex(sha.data(),sha.size());w.Raw("}");}
 w.Raw("],\"resets\":[");for(size_t i=0;i<std::min<size_t>(c.resetCount,c.resets.size());++i){if(i)w.Raw(",");PointJson(w,c.resets[i]);}w.Raw("]}");
 if(!w.Good())DiagnosticFallback(w,summary,"DIAGNOSTIC_BUDGET",limit,stale);
}
}}
