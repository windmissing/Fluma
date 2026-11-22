# T804 v1 · 目标 20 DM GPU 时间戳与独立前缀 fence 补证

日期：2026-11-22，effector，真机（011124264R000337）。
被测：T797 driver（buildId 09befda4，argv `--gpu-timing`/`--prefix=`
接受核验）。证据：`test/reports/t491/t804/`（g1-gpu-timing.jsonl、
p8/p9/pcopies/p11.jsonl、v1 判定）。

## G1：--gpu-timing（时钟可用性裁决）

argv 已传且被接受，但 **gpuClock=UNOBSERVED、GPU_SEGMENTS=None**——本
SoC 无可信 GPU 时间戳，**合同降级路径（独立 prefix fence）验证有效**。

G1 副产物：gpu-timing 插桩改变 guard 结构（DISPATCH_PRE PASS @8 →
DISPATCH_POST DEADLINE @8），exitCode=6，原 250ms 保持。

## 四独立 prefix fence（核心证据）

| prefix | exitCode | cursor | submitted | retire fence sig | retire polls |
|---|---|---|---|---|---|
| op8 | **0** | **9**/12 | 9 | ✅ | 157 |
| op9 | **0** | **10**/12 | 10 | ✅ | 189 |
| copies | **0** | **10**/12 | 10 | ✅ | **199** |
| op11 | **0** | **12**/12 | 12 | ✅ | 148 |

四 prefix 全部 exitCode=0（独立前缀模式成功）、cursor 与合同一致
（9/10/10/12）、**零 commit**（prefix 不走全管线提交）、**退役 fence
独立创建并 signal**（polls 计数逐 prefix 不同——即 fence 原值证据）。

## copy 成本下界（T799 触发条件达成）

copies（cursor 10，polls 199）vs op9（cursor 10，polls 189）——同
cursor 不同 fence polls；加上逐事件 ns 数组，T799 所需
COPY_COST 下界证据**齐备**。

## 回执

T773 reader 判定原件 t804/g1+p8+p9+pcopies+p11；归 architect（copy
成本与 GPU 时间戳不可用结论）。
