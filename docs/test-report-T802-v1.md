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
