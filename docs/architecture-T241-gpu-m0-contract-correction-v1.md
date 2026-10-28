# T241：M0 双 pad 存储与产品 wire 工程修订回执

2026-10-29，architect；原卡 `6ae20646e4b0c3103c15d490`，2026-10-28 开始。消费 T236 `c7ab688` 的 `docs/developer-T236-gpu-resident-M0-foundation-v1.md`、`test/reports/t236/sort-storage-gap.json`、`handoff-questions.json`，修订 T233 工程稿/manifest 并同步 T238 M1 继承账。不使用真机，不重跑 T236 测试或产品测试，不改业务代码、生产默认或其它清单行。所有渲染含背景/前景仅新 XComponent+shader，失败 FAULT 或显式 STALE 完整有效旧新管线帧，禁止 Canvas 上屏/回退。

## 归因与裁定

T236 报的是**工程文档缺口**，无需改 T230 数值/排序设计：K192 bitonic pad256 在 `k=128,j=64` 已有 64 个有效 ID 暂入 index192–255；T233 原 `two_index_arrays=8K=1536B` 仅够两份192 word，双 pad 需2048B。裁定显式扩容512B，两个完整256word pingpong 数组，逐层一对唯一 writer 写两端、barrier 后切读写，padding index i 存唯一 slot=i≥K、valid0/id0，真实 MAX_ID 由 valid 独立判。K512原有两份512word不变。source/quantum 设计 dispatch 上界268/306未因**存储扩容本身**改变；实际因 binding/对齐拆 pass 时须据实升版。

T233 V1 只给 event64/completion256 word index，未定产品数值；T236 的 TEST `CompletionPolicy` 不是产品 wire。现 T233 §6 和 manifest `engineeringRevision=T241_M0_WIRE_AND_SORT_V2` 首次冻结 magic/revision/seal、rootKind/outcome、terminal/reason、phaseSourceState/flags/namespace、M0七 kind used/zero mask 与 patch/replay、SUCCESS/PARTIAL/正常拒绝/同key replay/fatal 的 sourceSuccess 与前缀规则。尤其同key replay 的完成包可带**原 admittedUnits>0**，但此次 stock/commit/I/prefix 不增、`sourceSuccess=false`；原 terminal 不被重写。fatal 不接受 candidate。M0 bh 初始0，不接受漏层日志的非0基岩 fixture。字段 index 未移位，生成头必须绑定新 manifest SHA 重生成，旧 hash 不再是当前工程版。

修订载荷 K192 **15,060,496B**、K512 **36,997,904B**；单 event block、arena 行界、tail fence≤5与每root completion1024B不变。T238 M1 manifest/工程稿同步修订：K192/Kw64 **23,343,248B**、K192/Kw128 **25,472,400B**，K512 Kw64/128仍 **55,111,056/57,240,208B**；M1 revision2、water event kind 映射与 M0 revision1 分离，不能把 M0 kind2 MOVE 解作 M1 DIRECT_W。以上为设计 payload，非实际分配或设备通过。

交叉核 M1 还补了 `SOURCE_BIRTH` 的 stock A 前后 word32–33：沙源是 PH，飞行水包源是 J；DIRECT_W 是 W。这样 source event replay 同时证明 pool slot 与列库存的原子 patch，不会仅记录水包出生而漏 J。

## 静态核对与 T236 续做接点

本轮只做文档/JSON 静态交叉核：两档 M0 `sum(parts)=basePayload`、`two_index_arrays=8*Kpad`，四档 M1 等于 T237 设计账加 K192 512B 修订；WORK 两 sand index 各有 Kpad 个 U32、偏移连续且不别名；M0七类与 M1十四类 `usedMaskHex` 与索引集合一致；M0 revision1/M1 revision2、outcome/terminal/reason 数值域一致。**没有重跑** T236 的76/82205向量，不借其基础 PASS 证明 GPU source/step/scene 已完成；没有执行真机/GL shader。T232 XComponent/context/实际资源/同步能力仍按真实结果分支，不阻 T236 离线主核继续。

交 developer 继续原 T236（不使用真机）：重生成 manifest 绑定布局/资源和产品 `CompletionPolicy`，把双 pad read/write/bounds、事件逐kind编码及完整 replay、正常拒绝/重复请求回执门落实；随后补 GPU source、积分/DDA、LAND、床 guard、全 audit/replay、真实尾 GL fence、publication 与同 token 场景，编译及非真机自测。共享 `p013renderer/egl_context.*`、`scene_compositor.*`、`napi_bridge.cpp`、CMake hunk 只由单平台集成 owner 串行负责，不覆盖其它在途修改；feature0基础切片不得冒M0实现。独立 tester 的 GR01–09/设备、effector 同帧效果另卡；失败先 architect 归因，manager 登记接力不设验收门。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自己行状态有限例外；不改滴答同名清单。生产默认不迁、无 CPU 热路补算、不用 Canvas。
