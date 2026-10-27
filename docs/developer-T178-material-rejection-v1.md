# T178 开发交付

2026-10-27；依据 T176 §4/4.1；不使用真机。仅新增诊断，没有修数值、放宽 renderer 域或修改 physical/endpoint 协议。

原 FrameAdapter/MaterialData 的 if、检查顺序、四遍 Float64 平滑、F32 转换、配方、成功 data.set 均保留。在原 return/throw 旁写首拒绝点，在 ScenePreparation 原 catch 内 best-effort 冻结对应 publication，不重新 pack/step/snapshot/audit/copyStateTo，不更新派生场；外层仍 SCENE_MATERIAL_DATA / native code100。非预期 Error 保存 name/message，诊断异常不会替换原结果。

新增 `P013MaterialRejectionStore`，同 run 最早 **准备成功** 摘要、首次材料失败及最近8条摘要；重复同 reason/column/mask 只增 repeatCount，不再复制数组；新 run 清当前槽。摘要含 run/state/publication/fieldCommit/ledger/route/medium/page/context/scene/frameToken、session/context/surface/geometry/continuity、源/目标box/viewport/scale、pipeline/config binding/digest。原 build 身份不能由运行对象推出，明确 NOT_CAPTURED_USE_PACKAGE_MANIFEST，当前包身份另附 delivery-manifest。

固定原字节bank共28752B：10×128×F64场、2×128×F64池cache（合12288B），512×F32 display候选、1536×F32 material staging、4×128×F64 warp/spark、10×F64 motion、512×F64当前profile。数组元数据记录真实类型、length/offset/byteLength、同buffer bindingId及捕获前缀。无效旧staging/profile不冒当前；缺字段/捕获中断显式缺测。实际有效前缀先冻结，冷导出 SHA256/hex 保留 NaN payload、-0；源数组不写。捕获时只拷原buffer字节；导出列出由冻结bytes重新计算的6个RN加法中间量，均标 DERIVED，不能称原存储值。只捕获池cache，poolReplayComplete=false。

SOURCE mask：1非有限top、2非有限water、4非有限bottom、8非有限mix、16 bottom<0、32 bottom>mix、64 mix>top、128 top>water、256 water>capacity。F32 mask沿原实际谓词（不新增bottom/mix finite检查）。VALUE mask：1/2/4 warpH/L/M非有限，8 spark非有限，16/32 mh/ml非有限，64 spark<0，128/256 mh/ml<0。LAYOUT mask按代码对应boxW/H、mh/ml、warpH/L/M/spark；PROFILE布局另列cols/capacity/数组长度。未知列/通道用null，不能从总类推唯一原因。

native SceneReceipt 增诊断字段，不改旧字段判定/首错语义：compatibilityFailureMask bit0 LastGood-invalid、1 run、2 context、3 surface、4 geometry、5 continuity、6 size、7 box、8 mirror；保持原 Compatible 才 replay。replayAttempted、presentOldStatus、swapOldStatus 原返回，未执行null。原错误code/stage与unbind后错误分别存；可取得的EGL原值在原失败点已消费后的context.error读取，不额外清错误队列。未存储的GL原码明确 NOT_CAPTURED；不把枚举Status冒GLenum。新字段为sidecar，不令局部粒子/旧包冒CURRENT。

页面在原scene失败回执后只读导出：每frameToken一次，2048字符×最多64块，带part/parts/complete与总长度，超过上限标不完整，不逐帧全dump。完整冷接口 `getLastMaterialRejection()` 可取得同帧冻结证据。捕获/编码增加旁路成本，本卡未取得设备时钟/耗时，captureTimeMs=null+CAPTURE_TIMING_NOT_CAPTURED；不宣称零开销或实时性能PASS。Typed byte view捕获时构造仅作借读，不保留异步publication引用。

离线自验新增17/17：LAYOUT/VALUE/SOURCE层序/F32拒绝/最终overflow、原数组不写、NaN payload及-0 raw SHA、buffer复用不改首错、新run/8摘要/重复计数、未知异常/诊断异常、同token回执。原T56 5/5通过。真实SceneCompositor+主机GPU桩验证兼容位、未执行、旧Present/Swap失败首因与原T56全场景；真实NAPI桥主机测试通过，GPU为synthetic，未作设备GL声明。证据 `test/reports/t178/`。

Native、CompileArkTS、PackageHap 通过；完整assemble末段仍既有SignHap00303116，未改凭据。历史s150包/设备首错原bits未重造；本新诊断包与已保存历史身份不同。数值根因、实际GL错误/时长、设备首错待独立取证回architect，不据本自验称材质故障已修。
