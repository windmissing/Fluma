# T802 v1 · 目标 20 RX 侧车写盘修订真机取证——运行被设备休眠中断（如实）

日期：2026-11-22，effector，真机（011124264R000337）。
被测：T802-rx（T798 回据 9c9097d3 核验一致→signed 6eedc7fc 签 PASS，
writerRevision T798——T793 mkdtempSync 模板修复交付）。证据：
`test/reports/t491/t802/`（l1–l9 布局、v1 判定）。

## 执行（两次 Configure 尝试）

1. 首次：安装/启动正常，Configure 进入 SHADER_COMPILE 后 hilog 环形缓
   冲滚出（终态前）
2. 二次：解锁后见 **CANCEL_REQUESTED** 状态——进程 29672 存活，取消路
   径生效

## 中断原因（环境，非修复失败）

SOURCE 编译墙时 **~5.5 分钟** 超过设备锁屏策略实际生效时长（600s wake
设置下 swipe 唤醒在长编译期不足）——Configure 被休眠策略取消，**侧车
SAVED 证据未获**。写盘修复本身本轮未测到（无一次完整跑完）。

## 如实记录

- sidecar 文件不在文件域（运行未完成）
- T798 writer 修复未经完整验证（执行环境中断，非缺陷证据）
- 应用域日志两次被环形缓冲滚出（终态前未落盘）

## 前置（重试）

需唤醒策略绕行：编译窗口内每 ~60s 周期性 swipe/click 维持前台。tester
管线就绪，收到执行窗口即可复跑。


## v2 追加（2026-11-22 15:45，effector；唤醒绕行执行——应用存活但编译异常延长，sidecar 仍未获）

**唤醒策略绕行已应用**（每 60–90s swipe/click）：应用全程存活（对比 v1
被休眠杀路径），CANCEL_REQUESTED 发布后 worker 线程仍持续运行（51412
utime 159600→367764 ticks ≈ **35 分钟 CPU**）——编译真实执行（非休眠杀）。

**新问题**：本轮 SOURCE 编译墙时异常延长（**15+ 分钟 wall**，前几轮
84s 编译+235s 链接 ≈5.5min）——疑周期唤醒干扰引入热/主机争用。运行未
达完成，**sidecar 仍未获**（文件域仅 gpu-program-cache）。

**结论**：T798 writer 修复仍未测到（两次环境中断：v1 休眠杀、v2 编译
异常延长）。**前置（二选一）**：低干扰运行（起始单次唤醒+改设置延长
亮屏）或 developer 缩短编译时长。按卡面"缺证按原失败报告"如实记。


## v3 追加（2026-11-22 17:55，effector；T805 工件纠正后 sidecar 完整采获——writer 修复验证通过）

**T805 归因吸收**：v1/v2 错用 T798 default 包（无 SOURCE 导出标记）——
正确目标是 T798 **bootSource** 包（da5ec9b4，DSO f96d365d，内嵌 BOOT
22318B+SOURCE 41767B）。

T802-v3：bootSource 包签 PASS（f39bcb3a）→ 安装 → 单次 Configure
（pid 57798）**5.5 分钟完整跑完无中断** → PUMP code=6 DEADLINE。

**sidecar 完整采获（writer 修复验证通过）**：
- `g9rx-source-terminal.json`（12641B，sha c8a61f0e）——SAVED 日志行：
  G9RxSourceExport revision=T798 ticket=1 pid=57798 kind=terminal
- `g9rx-source-retirement.json`（sha 18cba1eb）——退役后自动写入
- schema=`RX_SOURCE_EXPORT_V1`，trace 含 firstReject（**PRE_FENCE_CREATE
  DEADLINE code=6**，deadlineNs 76698760560938/nowNs 76698804642500）、
  recentPass（PRE_FLUSH）、fences（execution null=deadline 在 fence 创建
  前；retirement create+wait EXIT）

**T793 mkdtempSync 修复 VERIFIED**。RX 首拒结构化原件齐备（T765/T785
reader 适用）。pulse age 可计算。
