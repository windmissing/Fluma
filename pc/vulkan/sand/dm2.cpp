#include "dm2.h"
#include <algorithm>
#include <ostream>

namespace grs::pc::sand {
namespace {
constexpr auto &profile=g9dm::kProfiles[0];
static_assert(profile.k==192 && profile.rootColdBytes==kDmColdRootBytes &&
              profile.fullColdBytes==kDmColdFullBytes);
constexpr size_t prefixWords=8192, descriptorStart=2048, descriptorStride=32;

bool Covers(const Slice &slice,size_t bytes) {
  return slice.buffer && slice.generation && slice.bytes>=bytes && bytes<=UINT64_MAX-slice.offset;
}
}

std::optional<DmColdPlan> PlanDmCold(const DmColdSources &s,bool full) {
  if (!Covers(s.work,profile.workBytes)) return {};
  DmColdPlan plan;
  Slice summary=s.work,controls=s.work;
  summary.offset+=profile.work[6].begin;
  summary.bytes=384*4;
  controls.offset+=profile.work[7].begin;
  controls.bytes=128*4;
  plan.copies[0]={summary,1024*4};
  plan.copies[1]={controls,1408*4};
  size_t offset=prefixWords*4;
  const std::array<Slice,10> spans={s.base,s.final,s.selected,s.authority,
      s.discovery[0],s.discovery[1],s.discovery[2],s.discovery[3],s.discovery[4],s.discovery[5]};
  const size_t count=full ? 10 : 4;
  for (size_t i=0;i<count;++i) {
    auto span=spans[i];
    const auto size=i<2 ? profile.stateBytes : profile.arenaBytes;
    if (!Covers(span,size)) return {};
    span.bytes=size;
    plan.copies[i+2]={span,offset};
    offset+=size;
  }
  plan.count=count+2;
  plan.bytes=offset;
  return plan;
}

bool WriteDmCold(std::ostream &out,const DmColdPlan &plan,const DmColdReadback &mapped,
                 uint64_t timeout,const g9dm::ValidatedRecord &proof,const void *owner,
                 const DmColdMetadata &meta) {
  const bool full=plan.count==12 && plan.bytes==kDmColdFullBytes;
  if ((!full && (plan.count!=6 || plan.bytes!=kDmColdRootBytes)) || !owner || !proof.ProducedBy(owner) ||
      !mapped.words || mapped.mappedBytes<plan.bytes || !meta.request || !meta.trace) return false;
  const auto &tail=proof.Words();
  const auto &p=proof.Permission();
  if (tail[0]!=g9dm::kCompletionMagic || tail[1]!=2 || tail[4]!=g9dm::kProfileCode ||
      p.parameterSha!=g9dm::HashWords(meta.config.data()+64,64) ||
      p.inputSha!=g9dm::HashWords(meta.input.data(),128) ||
      !std::equal(p.parameterSha.begin(),p.parameterSha.end(),meta.config.begin()+16) ||
      !std::equal(p.layoutSha.begin(),p.layoutSha.end(),meta.config.begin()+24) ||
      !std::equal(p.shaderSha.begin(),p.shaderSha.end(),meta.config.begin()+32) ||
      !std::equal(p.inputSha.begin(),p.inputSha.end(),meta.config.begin()+40)) return false;
  size_t end=prefixWords*4;
  for (size_t i=2;i<plan.count;++i) {
    const auto bytes=i<4 ? profile.stateBytes : profile.arenaBytes;
    if (plan.copies[i].destination!=end || plan.copies[i].source.bytes!=bytes) return false;
    end+=bytes;
  }
  if (AwaitReadback(mapped.wait,mapped.invalidate,mapped.device,mapped.copyFence,timeout,mapped.coherent,
                    mapped.memory,mapped.allocationBytes,mapped.atom,mapped.offset,plan.bytes)!=VK_SUCCESS)
    return false;

  std::array<uint32_t,prefixWords> prefix{};
  auto *h=prefix.data();
  h[0]=g9dm::kColdMagic; h[1]=2; h[2]=prefixWords; h[3]=g9dm::kSeal;
  h[4]=g9dm::kProfileCode; h[5]=full; h[6]=uint32_t(plan.count-2); h[7]=profile.k;
  using g9dm::Put;
  Put(h,8,plan.bytes); Put(h,10,meta.request); Put(h,12,meta.trace); Put(h,14,meta.createdNs);
  Put(h,16,p.identity.run); Put(h,18,p.identity.context); Put(h,20,p.identity.surface);
  Put(h,22,p.identity.pageSession); Put(h,24,p.identity.inputEpoch); Put(h,26,p.nonce);
  h[28]=tail[6]; h[29]=tail[37]; h[30]=tail[44]; h[31]=tail[46];
  std::copy(tail.begin(),tail.end(),h+256);
  std::copy(meta.input.begin(),meta.input.end(),h+512);
  std::copy(meta.config.begin(),meta.config.end(),h+640);
  std::copy(meta.quota.begin(),meta.quota.end(),h+768);
  std::copy_n(mapped.words+1024,512,h+1024);
  if (h[1408+127]!=1) return false;
  const std::array<uint32_t,1024> zeros{};
  for (size_t i=0;i<plan.count-2;++i) {
    const auto &span=plan.copies[i+2];
    const auto begin=span.destination/4, words=span.source.bytes/4;
    const auto *trial=i>=4 ? h+1024+(i-4)*64 : nullptr;
    const bool executed=!trial || trial[0]!=0;
    auto *d=h+descriptorStart+i*descriptorStride;
    d[0]=uint32_t(i+1); d[1]=trial ? uint32_t(i-4) : 0;
    d[2]=i==2 || trial ? 1 : i==3 ? 2 : 0;
    d[3]=executed ? 2 : 1; d[4]=trial ? trial[0] : tail[37];
    d[5]=trial ? trial[7] : tail[44]; d[6]=trial ? trial[8] : tail[46];
    Put(d,8,begin); Put(d,10,words); d[12]=1;
    std::array<uint32_t,8> digest{};
    if (executed) digest=g9dm::HashWords(mapped.words+begin,size_t(words));
    else {
      p011gpu::Sha256 hash;
      for (size_t pos=0;pos<words;pos+=zeros.size())
        hash.update(reinterpret_cast<const uint8_t*>(zeros.data()),std::min<size_t>(words-pos,zeros.size())*4);
      const auto raw=hash.finish();
      for (size_t j=0;j<raw.size();++j) digest[j/4]|=uint32_t(raw[j])<<(8*(j%4));
    }
    std::copy(digest.begin(),digest.end(),d+23);
    d[31]=g9dm::kSeal;
  }
  out.write(reinterpret_cast<const char*>(h),sizeof(prefix));
  for (size_t i=0;i<plan.count-2 && out;++i) {
    const auto &span=plan.copies[i+2];
    if (h[descriptorStart+i*descriptorStride+3]==2) {
      out.write(reinterpret_cast<const char*>(mapped.words+span.destination/4),std::streamsize(span.source.bytes));
    } else {
      for (size_t bytes=size_t(span.source.bytes);bytes && out;) {
        const auto amount=std::min(bytes,sizeof(zeros));
        out.write(reinterpret_cast<const char*>(zeros.data()),std::streamsize(amount));
        bytes-=amount;
      }
    }
  }
  return bool(out);
}
}
