#include "r1.h"
#include <iterator>

namespace grs::pc::sand {
namespace {
constexpr auto &visual=g11vis::kVisualResources[0];
constexpr size_t markerBytes=128*visual.v, glyphBytes=32*visual.v;
constexpr size_t planBytes=2097152, mapBytes=524288;
static_assert(visual.parent==0 && visual.k==192 && visual.v==2048 && visual.workBytes==4825728);
static_assert(4096+2*markerBytes+planBytes+mapBytes==kR1WitnessBytes);

bool Range(const Slice &s,size_t bytes) {
  return s.buffer && s.generation && s.bytes>=bytes && bytes<=UINT64_MAX-s.offset && s.offset%4==0;
}
}

std::optional<WitnessPlan> PlanR1Witness(Root root,WitnessSelection selection,const Slice &work,uint32_t bank) {
  if (bank>2 || !Range(work,visual.workBytes) || (selection.role!=2 && selection.role!=4) ||
      selection.phase<1 || selection.phase>6) return {};
  const auto *passes=root==Root::Source ? kSource : kQuantum;
  const size_t count=root==Root::Source ? std::size(kSource) : std::size(kQuantum);
  size_t begin=count,apply=count;
  for (size_t i=0;i<count;++i) {
    const auto &p=passes[i];
    if (!p.visual || p.role!=selection.role || uint32_t(p.micro)!=selection.micro || p.phase!=selection.phase)
      continue;
    if (!p.hasModule) return {};
    if (p.opcode==513) {
      if (begin!=count) return {};
      begin=i;
    }
    if (p.opcode==514) {
      if (apply!=count) return {};
      apply=i;
    }
  }
  if (begin>=apply || apply==count) return {};
  const auto span=[&](size_t offset,size_t bytes,size_t destination) {
    return CopySpan{{work.buffer,work.offset+offset,bytes,work.generation},destination};
  };
  const auto control=visual.visual[g11vis::kSegControl];
  return WitnessPlan{begin,apply,span(visual.visual[bank],markerBytes,4096),{
    span(visual.visual[bank],markerBytes,4096+markerBytes),
    span(visual.visual[g11vis::kSegMaterialPlan],planBytes,4096+2*markerBytes),
    span(visual.visual[g11vis::kSegFourMaps],mapBytes,4096+2*markerBytes+planBytes),
    span(control+bank*512,512,1024),
    span(control+g11vis::kControlPlanHeader.byteStart,g11vis::kControlPlanHeader.bytes,1536)
  }};
}

bool R1SceneGate::Acquire(const g11vis::ValidatedCompositeAccepted &proof,const R1Scene &scene,
                         const WindowKey &current) {
  const auto &permit=proof.Permission();
  if (scene_ || !scene.producer || scene.producer!=proof.Producer() || permit.parent.index()!=0 ||
      scene.leaseSerial<=previousSerial_ || !(scene.key==current) || !current.swapchain ||
      !Range(scene.state,g9spf::kProfiles[0].stateBytes) || !Range(scene.markers,markerBytes) ||
      !Range(scene.glyphs,glyphBytes) || !Range(scene.styles,g11vis::kControlPacketStyle.bytes) ||
      scene.markers.buffer!=scene.glyphs.buffer || scene.styles.buffer!=scene.glyphs.buffer ||
      scene.markers.generation!=scene.glyphs.generation || scene.styles.generation!=scene.glyphs.generation)
    return false;
  const auto &parent=std::get<g9spf::Permit>(permit.parent);
  const auto &tail=proof.Words();
  if (current.run!=parent.identity.run || current.owner!=parent.identity.owner ||
      current.context!=parent.identity.context || current.window!=parent.identity.window ||
      current.surface!=parent.identity.surface || current.permission!=parent.identity.permission ||
      current.geometry!=parent.geometry || scene.nonce!=permit.Nonce() ||
      scene.commit!=accept647::Pair(tail.data(),30) || scene.acceptedQuanta!=accept647::Pair(tail.data(),54) ||
      scene.acceptedQuanta>UINT64_MAX/256 || scene.visualTicks!=scene.acceptedQuanta*256 ||
      scene.visualGeneration!=permit.visualGeneration+1 || scene.visualGeneration==0) return false;
  scene_=scene;
  previousSerial_=scene.leaseSerial;
  presented_=false;
  return true;
}

bool R1SceneGate::CanPresent(const WindowKey &current) const {
  return scene_ && scene_->key==current;
}

bool R1SceneGate::BindGlyphs(const WindowKey &current,PFN_vkCmdBindVertexBuffers bind,VkCommandBuffer cb) const {
  if (!CanPresent(current) || !bind || !cb || fence_) return false;
  bind(cb,kR1GlyphBinding.binding,1,&scene_->glyphs.buffer,&scene_->glyphs.offset);
  return true;
}

bool R1SceneGate::Submitted(const WindowKey &current,VkFence fence) {
  if (!CanPresent(current) || fence_ || !fence) return false;
  fence_=fence;
  return true;
}

std::optional<PresentedMapping> R1SceneGate::Presented(const WindowKey &current,const PresentReceipt &r) {
  const auto present=[](VkResult value) { return value==VK_SUCCESS || value==VK_SUBOPTIMAL_KHR; };
  if (presented_ || !CanPresent(current) || !fence_ || r.fenceObject!=fence_ ||
      !present(r.acquire) || r.submit!=VK_SUCCESS || r.fence!=VK_SUCCESS ||
      !present(r.present) || !present(r.imagePresent) || r.imageIndex==UINT32_MAX ||
      !r.enterQpc || r.exitQpc<r.enterQpc) return {};
  presented_=true;
  return PresentedMapping{scene_->nonce,scene_->commit,scene_->visualGeneration,scene_->leaseSerial,
                          r.imageIndex,r.enterQpc,r.exitQpc};
}

bool R1SceneGate::CancelUnsubmitted() {
  if (!scene_ || fence_) return false;
  scene_.reset();
  return true;
}

VkResult R1SceneGate::Retire(PFN_vkWaitForFences wait,VkDevice device,uint64_t timeout) {
  if (!scene_ || !fence_ || !wait || !device) return VK_ERROR_INITIALIZATION_FAILED;
  const auto result=wait(device,1,&fence_,VK_TRUE,timeout);
  if (result==VK_SUCCESS || result==VK_ERROR_DEVICE_LOST) {
    fence_=VK_NULL_HANDLE;
    scene_.reset();
  }
  return result;
}
}
