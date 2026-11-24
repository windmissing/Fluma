# T949 · 目标 6/9/13 集显候选与 G16 接触实测（effector）

2026-11-24。开发集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924）。T946 工件（PE `bc056740`、DM2 split-v2 `c25db32d`、G16 探针 `1299d3fd`，SHA 全核过）配 T948 判据。GPU 驻留、shader 渲染，无 Canvas 回退。证据根 `test/reports/t949/runs/`。诊断结果不代替正式接受。

## ① 目标 6：PREPARE/并行库存候选四臂（goal6）

四臂私有 OFF 同参考交错（base35/35-parallel/base38/38-parallel）：

| 臂 | 结果 | admitted | 派发 | Q host record→stop | GPU span 中位 |
|---|---|---|---|---|---|
| base35 | COMPLETE | 126,053 | 35 | 36.1ms | 56.4ms |
| 35（并行库存） | COMPLETE | 126,053 | 35 | 31.5ms | 58.9ms |
| base38 | COMPLETE | 126,053 | 38 | 33.7ms | 50.5ms |
| 38（并行库存） | COMPLETE（重试 1 次） | 126,053 | 38 | 26.4ms | 48.6ms |

- **业务等价**：**`source-ledger.u32le` 四臂逐字节相同**（入料账全等）；其余件差异词全部落在身份/时钟/密封哈希区（completion word4/6/8+114-121、work 7172/7256-63、publication 7684/7721-28、events word2——每根唯一字段）。
- 38 臂首跑 `TAIL_REJECTED` 一次（瞬态，重试过）。
- **结论：并行库存候选与基线业务等价，真实时标全档。**

## ② 目标 9：三臂分段计时与裁槽前置证据（goal9）

| 臂 | 结果 |
|---|---|
| source-only（steps 0） | SOURCE vk=2（边缘摆动） |
| q-control（steps 1） | SOURCE vk=2（边缘） |
| q-timing（steps 1 + --quantum-timestamps） | **SOURCE 接受**（14 派发 vk=0）+ QUANTUM `GRANT_DEADLINE`（2404 派发、submit→stop 恰 250ms、tailRead=false——T923 晚尾模式） |

- **GPU 时标**：SOURCE 6 点 + **QUANTUM 15 点**（分段计时：upload/discovery/selected/authority/rejected-prefix/replay/pack/seal/tail），绑定真实业务 fence 代 4、查询池+reset serial。
- **空微步 control[16]（裁槽前置直接证据）**：QUANTUM 拒根 ACTUAL word16=**2**、BASE=1——**拒绝的空微步量子仍逐槽递增 control[16]**（T946 文档预判的实测确认）。
- 双态 raw 封存：SOURCE accepted 20 件 + QUANTUM diagnostic 19 件（rootVk=2）。
- **结论：同 fence latch、空微步递增、拒绝前缀与迟到原件全采——交 T946 裁槽决策（选择有收益且可保计数的单个 extent）。**

## ③ 目标 13：三闭包资格与 F02（goal13）

- **资格**：旧 `OP_48` 单独跑仍 `CALL_UNRETURNED`（旧模块 igc 挂死在本机持续——T912/T921/T937 模式）；新角色名（OP48_VISCOSITY/OP49_PRESSURE/OP48_REMAINDER）仅在 `--g13-split-op48` 下有效（t833 重命名机制），standalone 资格名被用法拒。
- **F02 original**：240s 超时（旧 OP_48 编译挂死）。
- **F02 split**：**三闭包管线实建**（34 条 STEADY_PIPELINE；OP48_VISCOSITY×10（812db31f）、OP49_PRESSURE×10（ca603a59）进度行）；根 `T833_ROOT_RETIRE_WAIT FAILED`（vkWaitForFences 迟到根——T901 F02 模式一致）→ 818 audit 与压力/速度/散度/黏性边沿证据未达（需接受根）。
- **结论：部分**——闭包管线 live 资格过；F02 物理证据被迟到根挡（集显），旧 OP_48 挂死仍在。

## ④ G16 接触修订候选（g16，模块 792f5986）

| 臂 | T939 旧失败 | T946 新结果 |
|---|---|---|
| B13 static ×2 | reason 10/4 bracket unresolved | **消失** → 新门 reason **11/9**（余时核算：物理推进循环后 remaining>0）；B=200 C=27 **Ic=32**（旧 0）、20.8ms fresh=true |
| B52 static/fast | reason 14/8 求解残差 | **消失** → 新门 **超窗**（vk=2、256.9ms > 250ms、fresh=false、B=0 无边——52 盘×256 微步超预算 ~7ms，非物理拒） |

- **结论：T939 两个旧失败均被修订消除**；新阻塞为 B13 时间核算门与 B52 集显超窗（7ms）——正例接受仍未达，但性质从"物理层错误"变为"预算边缘"。

## 结论

| 子项 | 结果 |
|---|---|
| 目标 6 并行库存 | **过**：四臂 admitted 全等+ledger 逐字相同+真实时标 |
| 目标 9 分段计时 | **过（诊断）**：15 点 QUANTUM 时标+control[16] 递增证据+双态封存，交 T946 裁槽 |
| 目标 13 三闭包 | **部分**：split 下闭包管线 live 资格；F02 证据被迟到根挡；旧 OP_48 挂死持续 |
| G16 修订候选 | **关键进展**：两个 T939 旧失败消除；新门为余时核算（B13）与 7ms 超窗（B52） |
