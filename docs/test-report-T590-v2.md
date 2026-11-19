# T590 v2 · a2 干净构建包设备续证——boot 诊断/recovery 状态机/三 attempt 终态原件

日期：2026-11-19，tester，真机（011124264R000337）。
被测：T590-a2-signed.hap（t541-t624-a2 树=T564 真源+干净 cache=T655 归因续办
构建，inputSha 6b019ef9→signed 5dcaeda5，sign=PASS）。
证据：`test/reports/t491/t590c/`（t590-boot.json=T451_GPU_BOOT_PROBE_V1 完整
原件、l1/l2 布局、v2-a2.json）。

## 原件核心（boot 诊断，pid 14871）

**recovery 状态机**（T464_RECOVERY_V1）：
`phase=CREATING_NEW / firstReason=WINDOW_OR_PAGE_CHANGED / resetSerial=1 /
epoch=3 / retired=true / resources=false` ——reset 链字段完整前进。

**三 attempt 终态**：
| bootSerial | terminal | complete | firstFailure.stage |
|---|---|---|---|
| 1 | true | true | SURFACE_CREATED |
| 2 | true | true | RESET_REQUIRED |
| **3** | **false** | **false** | （无——创建臂活动中） |

transitions 样本：XCOMPONENT_UNWRAP/REGISTER/SIZE→CALLBACK_WINDOW_REFERENCE→
SURFACE_CREATED（epoch 1→2）→UI_RESET（epoch 2→3）——surface/window/epoch
链逐项可追溯。

## 判定

- **attempt 3 非终态=创建臂活动**——与 T593 v3 的 GPU_WORK_PENDING↔
  DRAW_PENDING 振荡同族（同包内 clock V2 域）
- recovery 状态机 resetSerial/epoch 正确前进（T664"恢复链未证缺失"在此包
  得到正面观测：链活着，卡在创建臂 terminal）
- 同 attempt 拒因现按 attempt 逐项具名（SURFACE_CREATED/RESET_REQUIRED/
  活动中）——设备期限域（250ms root deadline 不变）在此结构下可精确归属

## 回执

a2 包（干净 cache 链）部署成功=工具链修复验证第二证；boot 诊断原件+recovery
状态机+三 attempt 终态交付。与 T591 v3 LINK 40s / T496 崩溃 / T593 v3 振荡
四证据汇聚——创建臂 terminal 缺失为 M1 全系共同根因候选，回 architect。
