# T737 · G13 BOOT 次正规数拒绝与签包回执核对

2026-11-20 · 目标20 · architect · 离线归因完成。

**Attempt 250 首先被 op65 拒绝。op66 两轮独立探针也都失败。** 同票原件显示，次正规输入加零与次正规结果生成都回读为零，其他已采哨兵符合原门。这次运行中的实际 BOOT GLES 程序呈现次正规数清零。驱动内部成因和冷暖稳定性待证。已签 SPLIT 包内容与 T732 编译产物一致。签包预检失败来自回执的换行与角色描述，修订回执可通过严格预检。 [机器证据](architecture-T737-evidence-v1.json)

## 同票原字和实际首因

配置原返回 QUEUED 与终态共用 boot 5754112216094、attempt 250。终态另见 queue 3/run 1/context 1、firstFailureAttempt 250、bootReadyNs 0。BOOT 源码 SHA 为 a93aecf86ddb1f782b337067f5b5c54fe7ac5676e2f9c24d8c25f8c8af4b24bb。fence 0x911a（GL_ALREADY_SIGNALED），firstGl 与 WORK 回读 GL 都为 0。冷尾 [255] 为 0xc0de1301，[7] 为 27。原件提供票据与 BOOT 源码关系。本次 PID/start 与实机映射 BuildId 尚缺。 [Configure 原返回](../test/reports/t491/t730b/v5/t732-diag.json) · [终态布局原件](../test/reports/t491/t730b/v5/l2.json)

| 检查 | 原阈值或位型 | 实测 |
| --- | --- | --- |
| op65 WORK +35/+41/+47，sin(0)、cos(0)、sin(π/2) | 0、1、与 1 的绝对误差 ≤ 2⁻²¹ | `00000000 / 3f800000 / 3f800000`，通过 |
| op65 WORK +53/+59，次正规输入加零与 1 的半 ULP tie | `00000001 / 3f800000` | `00000000 / 3f800000`，+53 拒绝 |
| op66 每轮 8 words | `3f800001 3f800000 00000001 00400000 00000007 00000000 00000000 46503332` | 两轮均为 `3f800001 3f800000 00000000 00000000 00000007 00000000 00000000 46503332`，索引 2、3 拒绝 |

op65 在 coldAudit 命中原 subResult 谓词。实际 BOOT 内只有该位置写 fail(27u)，尾部从首错 c[514] 生成。op66 另存两轮签名，分别证实最小次正规输入 0x00000001 + 0 和最小正常数 0x00800000 乘 0.5 的次正规结果清零。常规 ULP 加法、tie、分类位与两封印均正确。宿主 Poll 先记录两个阶段，再按 auditFailed 返回尾部 27。原尾合格才可能由后续 !qualified 拒绝。INIT_FP32_RESULT:EXIT 是最后诊断时点，本次生效的拒绝仍来自 op65。T730 v5 的“已越过 op65、只剩 op66”应以原字修订。T732 修补使此次取证可见，未进入稳态角色资格、COMPUTE_READY 或首 Swap。 [op65](../entry/src/main/cpp/gpu_resident/g13flow/units.glsl) · [op66](../entry/src/main/cpp/gpu_resident/g13flow/pipeline.comp#L94) · [宿主拒绝顺序](../entry/src/main/cpp/gpu_resident/g13flow/transport.cpp#L665) · [T690 原门](developer-T690-v1.md)

## 对象、偏移和同步

- Create 在同 GL owner 和同 BOOT program 上完成。CTRL 为 18,688 words / 74,752 B，绑定 3。宿主先上传完整 CTRL，输入位置 744–747 和 18648–18652 均在范围内。op64 建初态并清 WORK。T732 在 GPU bank 写完、CPU 重叠上传 header 之前加 BUFFER_UPDATE barrier。op65 使用 bank0、WORK binding2 和 CTRL binding3。op66 不换 program 或这些绑定，用 scope 0、1 各跑一次，每次后保留 SSBO 与 BUFFER_UPDATE barrier，统一 fence/Flush，完成后才 map。
- BOOT CTRL map 的 1024 B 区间为 [73728,74752)，两组签名在其中 [74528,74592)，分别是 18632–18639、18640–18647 字，紧邻的输入从 18648 开始，互不重叠。尾部 error 在 byte73756，seal 在 byte74748。T732 修正的历史双乘四属于后续稳态角色回读，attempt 250 尚未进入该路径。WORK 另用其 GL 对象，自 work[19].offset + 512 字读 60 words。默认 REST 64×64 的区间是 [583296,583536)，五个相对槽是 35/41/47/53/59。实际 profile 从原配置与 owner 收据绑定。
- 成功 map/unmap、两组哨兵及独立 WORK 的一致结果支持实际浮点行为。GLSL ES 3.10 允许次正规输入或运算结果被清零。highp 单独无法保证 T690/T607 合同。本次判该程序与运行环境未达原资格。厂商内部成因、全局硬件能力和 Vulkan 支持仍需单独证据。 [T732 修订](developer-T732-v1.md) · [Khronos GLSL ES 3.10 §4.5.1](https://registry.khronos.org/OpenGL/specs/es/3.1/GLSL_ES_Specification_3.10.pdf)

## 签包和角色回执

| 工件 | HAP SHA256 前缀 | 包内 ARM64 BuildId |
| --- | --- | --- |
| T732 SPLIT unsigned | 2b84038a | 7154816ed3d2d13781d66670a1a8e4acb8fa0eb0 |
| T732 SPLIT signed | c18addcf | 7154816ed3d2d13781d66670a1a8e4acb8fa0eb0 |
| T732 MONOLITH unsigned | aa648d58 | d03acdb620b9006690eece8cfe502fb00645ecc8 |

打开原 HAP 逐字节核对 SPLIT 的 14 个载荷条目，签前后全同，包内 ELF SHA 为 3c4d4a20…。签名原回执 verifyApp PASS。两臂的 scene.vert 树文件均为 **1,697 B / bf7b11aa…**，正好有 23 组 CRLF。生成器规范为 **1,674 B / 478d2e92…**，与两臂内嵌 LF 源逐字节相同。scene.frag 同样有 12 处 CRLF，1,529→1,517 B，规范 SHA 为 c9cdc98f…。实际送给编译器的源字节和 T732 compile.json 值一致。

原 T732 两臂适配缺 sourceNewlines=CPP_TRANSLATION_LF 与原始 sourceSHA256，导致预检直接拿 LF SHA 核 CRLF 文件。原角色描述也有误。graphics 标成 vertex/fragment 并挂全 opcode，拆分 OP_* 标成 fragment，BOOT/各角色误挂全表，缺 reworked。修订从各自实际构建的 assets_manifest.json 导出 compute/graphics、准确的正常派发 opcodes、限额标记、原文件/规范字节 SHA。BOOT 的正常 opcode 是 64/65/66，稳态程序的私有资格 opcode 66 单列 qualificationOpcode。glShaderSource 精确 SHA 仍过 ELF 原 bytes+NUL 检查。严格预检重跑原 sidecar 仍报 scene.vert SHA_MISMATCH，临时修订两臂 **verify-only 通过**，覆盖 0–66、同父、不同包、相同图形源和 48 KiB 上限。交 developer 将适配修进生成收据，effector 用原严格签包入口。 [签包原回执](../test/reports/t257/T732-split.json) · [编译原回执](../test/reports/t732/compile.json) · [旧预检拒绝](../test/reports/t730b/v4-t732-pair-check.json) · [T729 预检](../test/t729/sign_pair.py)

## 工程交接与最短复验

1. **给 op65、op66 各自的终态。** 在原 Poll 回执增加 effectiveRejectStage，分别记录 op65 sealed/error/原 WORK 谓词、op66 双轮 presence/qualified/错位 mask 及原 GL/EGL。op65 原拒绝优先，op66 独立留证。采用版本化附属层，保四页 ABI、原顺序和阈值。绑定 ticket、queue/run/context/surface、线程、实际 program/CTRL/WORK 对象、fence、映射 BuildId。只从成功 map 的对象记录原字，失败保原错和实际阶段。签包工件从生成器直接导出具名角色。
2. **一次同票收口 GLES。** 固定 T732 SPLIT REST 64×64，原 BOOT 字节和 T690 门，单次 Configure。取 PID/start、映射 libgrs_g13 BuildId/SHA、GL/EGL 指纹、实际源码或 binary 路径与 binary SHA、两轮原位、CTRL/WORK 区间和 fence。同一 fence 后额外读 CTRL [744..747]、[18648..18652] 的真实输入字。若输入与结果矛盾，加独立标识、同 owner 的 uint echo 短对照，分离上传/映射与浮点执行。优先复用已匹配的 warm artifact 收同一次回执，若需对比源码与 binary 再补无该 key 的源码路径。再次获得原字即登记该 GLES 环境 FP32 资格失败。到时截尾保真实最后阶段。
3. **并行推进 T731 的 Vulkan BOOT 对照。** 同机先读 API/driver 和 VkPhysicalDeviceFloatControlsProperties 的 shaderDenormPreserveFloat32、shaderRoundingModeRTEFloat32、shaderSignedZeroInfNanPreserveFloat32。核心版本或 VK_KHR_shader_float_controls 与相应 32 位属性均成立，再用固定工具链生成原同源 BOOT，将 DenormPreserve 32、RoundingModeRTE 32、SignedZeroInfNanPreserve 32 写入最终 SPIR-V entry point。校验实际模块模式、探针运算不受 RelaxedPrecision 影响、原 no-contraction 及真实 CTRL/WORK descriptor 与 upload/compute/readback 同步。最小闭环只跑原 BOOT 与同阈值 65/66，双轮原位吻合且 ctrl7=0 后再推进逐角色、非零首根与原 XComponent present。属性缺失、pipeline 拒绝或原位不符时记录具体失败，保原算法、GPU 驻留和 XComponent+shader。后端切换以实测通过为据。 [T731 最小对照](architecture-T731-v1.md) · [Khronos Float Controls](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceFloatControlsProperties.html) · [SPIR-V 执行模式](https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_float_controls.html)

历史 T730 冷暖记录只证 23,834 B **BOOT**。同 key、同 SHA 和 301,961 B 的 binary，冷 compile 49.850 s、link 66.635 s、总 build 116.491 s，后次 load 5.645 ms、build 10.259 ms。缺 attempt 250 的配对缓存事件与 fresh PID/start。BOOT_READY、COMPUTE_READY 和首个有效 Swap 均未达到。本卡未使用真机、未签包、未调整资格阈值。 [历史缓存原件](../test/reports/t491/t730b/split-cache/856a-warm.jsonl)

离线复核命令为 `python tools/architecture/analyze_t737.py`。
