# T285 V2 有界冷采真机续证（真机 BLOCKED：新热路径阻塞缺陷）

2026-10-30，tester 真机（VYG-AL00 62T0225B10010020）。包：T284-signed.hap，SHA256 `ed16f0660a544f98…`（实测一致），新版 `test/t284/reassemble.cjs` 就绪。

## 一、T284 修复确认（相对 T277）

- **t276Cold 每帧冷采阻塞已消除**：本轮两个 run 的 THREAD_BLOCK_6S 栈中**无任何 t276Cold/T276Export 帧**（T277 时代栈顶为 SharedHeap←t276Export←t276Cold←tick）。
- 冷采未再淹 hilog（本轮 T276_COLD 分片 0 条直到死亡，符合"限次冷采"设计）。

## 二、新缺陷（独立复现，回 architect）：粒子 patch 热路径 ownsBufferHot 主线程阻塞

两个独立 run 同栈被 watchdog 杀死：

- run A（pid 62689）：注入 60s 期间死于 12:26:08，`THREAD_BLOCK_6S-62689`
- run B（pid 64350）：注入 30s 期间死于 12:29:13，`THREAD_BLOCK_6S-64350`

两份 faultlog 栈完全一致（`test/reports/t285/tb62689.log` / `tb64350.log`）：

```
#00 BCStub_HandleLdaV8StwCopy（GC/字节码 stub）
#01 ownsBufferHot entry (P011ParticleHot.ets:391)  ← bufferBindings 线性扫描
#02 fieldStorageValidHot (P011ParticleHot.ets:478)
#03 patchLeaseHot (P011ParticleHot.ets:1103)
#04 finishPatchHot (P011ParticleHot.ets:1125)
#05 applyInto (P011ParticleM2Sink.ets:292)
#06-08 applyParticlePlanInto → apply (P011ParticleM2Field.ets:321)
```

机制：注入产生大量粒子 patch 计划，`applyParticlePlan` 每次经 `patchLeaseHot→fieldStorageValidHot→ownsBufferHot` 对 `bufferBindings` 做线性扫描；在 patch 高峰期（SharedHeap Used 7.3MB、DireAllocate 215MB 压力）单次事件处理超 6 秒 → watchdog signal 9。

## 三、T285 目标达成情况

| 项 | 结果 |
|---|---|
| 单 run FIRST_BIRTH | ✅ run `p011-run-1793334398715-3` 场景事件 CURRENT 连续（commit 1532→1802、frame 765→889），swap=true/staleSwap=false |
| 源停后持续 clock | ⚠️ 注入停止后进程被杀（12:29:13），clock 延续未及采样 |
| since≥8s 成功物理秒 | ❌ NOT_RUN——两 run 均死于窗口内（watchdog），非低电/非 Doze（CPU TIME 活跃） |
| LAND writer→床量→publication→同 token 像素 | ❌ NOT_RUN |
| reassemble 工具链 | ✅ 就绪未触发（无 cold 分片产出——限次冷采在 8s 前未到触发点） |

不冒 PASS、不推唯一因（watchdog 杀因明确为 ownsBufferHot 阻塞，但为何本次注入规模触发而 T277 时代未触发，需 architect 结合 T259/T279 patch 计划规模裁决）。

## 四、结论

**BLOCKED 待 architect**：新缺陷（patch 热路径 ownsBufferHot 线性扫描在高峰期阻塞主线程 >6s）修复后重跑即可——包/工具链/流程全就绪，与 T277 的 cold 阻塞已解除是独立事实。

证据：`test/reports/t285/`（两份完整 THREAD_BLOCK_6S faultlog）。
