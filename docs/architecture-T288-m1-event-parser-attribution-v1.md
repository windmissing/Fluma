# T288 M1 候选事件独立解析首次归因 v1

2026-10-30，architect；原卡 `6ae3f6c7e4b031ea82c25c23`。不使用真机、不重跑测试。消费 `docs/test-report-T287-m1-candidate-indep-v1.md`、`test/reports/t287/t287_oracle.py`、T279 已存二进制、`test/gpu_resident/t279_quantum_host.cpp`、`test/gpu_resident/check_t279_quantum.py` 与 T238 manifest。本稿仅分流 T287 的独立解析争议，不修改 M1 产品 wire 或 T280 在途开发。

## 原件与单位核对

- `QuantumCandidate::Capture()` 冷读 `status[1]*64` 个 **U32 word** 到 `log`；host 写出 `info[2]=events.size()`，再按 `events.size()*4` 字节写二进制。`check_t279_quantum.py` 先取 `info[2]` 个 word，再每64 word切一个 event。T238 manifest 同样冻结 `event64BytesPerRow=256`，即64 word。
- T287 oracle 已正确用 `off+=info[2]*4` 跨过整个事件段，却在逐行读取时写成 `struct.unpack_from('<64I', raw, off+j*64)`。`unpack_from` 的偏移单位是**字节**，故第 j 行应从 `off+j*64*4`（或 `off+j*256`）读取。当前脚本从0、64、128、192 byte起取各256 byte，窗口互相重叠，不能当连续 event64 行。
- 直接按既有二进制的 word 偏移检查 case33，`info[2]=256`，四个真实 event 的 magic 位于 word `[0,64,128,192]`，kind `[4,7,11,11]`。T287 脚本把重叠窗口拼接后观察到的 `[0,112,160,208]` 是解析产物，不是产品 112/48/48 word 混合步长。case21 实际有三个 WATER_EDGE kind10 行，首行 `64→63`，还存在 `63→62` 和 `64→65`；报告“唯一行”亦由只看到重叠解析的首行造成。

因此，**没有证据表明产品 Capture 使用未文档化的48-word插行、布局与 checker 同源自洽，或 event64 wire 错位**。T238 规格已足够锚定64-word stride；问题属于 T287 独立测试脚本的字节/word 单位错误及由此产生的用例结论，交 manager 裁决重跑独立 B/C/F 与 E 全流。无需为此先改产品布局或派 developer；T280 与本次 parser 修正可并行，不增开发验收门。

## T287 已有结论的有效边界

- A 的失败 bank 与输入逐 word 相等、G 的 status reason 来自事件段外，仍是独立抽核的窄证据；152/0 是重跑既有开发 checker，不能充 B/C/F 独立验证。
- D 的两例 CROSS 第一行恰好从正确段起点读取，已报告的单行预算数值可保留；不外推整个事件流均通过。
- E 的首个 WATER_EDGE `64→63` 数值可保留；“唯一行”与整个水边写集 PASS 撤销，须按三个真实行和独立 head2/顺序预期重核。
- B T16、C ARRIVE 顺序、F event64 used-mask 仍 `NOT_INDEPENDENT`；它们既非产品 FAIL，也非独立 PASS。尚无 T280 accepted bank、产品 scene、真机或收益证据。

## 给 manager / tester 的精确接力

manager 将 T287 的解析/用例争议交 tester 修正并复核，保留原执行报告与错误观察的勘误。tester 在独立 oracle 中先 assert `info[2]%64==0`、`status[1]==info[2]/64`、`off==len(raw)`，一次读取整个事件段后按64 **word**切行，或以 `off+j*256` byte读取；每行核 magic/revision、ordinal、kind、未用位，并从 T238 manifest 自己生成 mask 与字段偏移，不调用开发 checker。对 case21 核全部 WATER_EDGE 行和前后库存；对 case33 核两 ARRIVE 的 `(column,y,id)` 序与 J→W；B 核正确行的 T16 RNE/τ/medium，F 逐 kind 核 used-mask/保留0。重新报告各项分母/结果及原 T287 D/E 勘误，失败再附原 word offset/首个不同字段回 architect。不得用 magic 扫描代替明确长度/stride。此为离线工作，不使用真机。

所有后续渲染仍仅 XComponent+shader，含背景与前景；失败 FAULT 或完整旧 token 显式 STALE，禁止 Canvas 上屏/回退。
