# T541 v2 · 三态差分真机首测——DM 包构建缺陷 + SPF 基线 RESET_REQUESTED 恒持

日期：2026-11-20，effector（tester 兼），真机（011124264R000337）。
被测：T670-dm-final-signed（SHA 79769acc）+ T670-spf-final-signed
（SHA f1994d31）——T670 离线差分设施交付包。
证据：`test/reports/t491/t541b/`（jscrash-latest.log、spf-boot.json、
v2-spf-baseline.json、l1–lF 布局原件）。

## DM 臂：包构建缺陷（NOT_RUN 如实）

T670-dm-final 包内 **缺 libgrsm0.so + libgrsm1.so**（仅 libgrs_dm_t647.so）——
EntryAbility onForeground 直接 import grsm0/grsm1 → dlopen 失败 →
`Cannot read property setActive of undefined`（onForeground:489，jscrash
4 次：07:48–07:51 稳定复现）——**T670 DM 离线构建产物缺 so**，回
manager/architect（构建缺陷，非产品语义）。

## SPF 臂：基线原件已锁

T670-spf-final 包 so 齐全，M1 页正常（CLOCK_V2）：
- recovery：phase=RESET_REQUESTED / reason=WAIT_RETIRED /
  firstReason=WINDOW_OR_PAGE_CHANGED
- 三 attempt 全 terminal/complete：1=SURFACE_CREATED、2=RESET_REQUIRED、
  3=UI_RESET——但 readyEpoch 全 0（**READY 未发布**）
- UI 恒持 RESET_REQUESTED（25s+ 不迁移），physics 0:0、收据 #0

## 判定

- SPF 臂卡在**旧资源退休未完成**（WAIT_RETIRED）——与 T590 v3 glLinkProgram
  开放、T593 v4 READY 已发布（同包族不同 run）形成三态对照素材：
  SPF 冷启动卡退休，正式 T597-fixed 能走完——差分差值在退休路径
- DM 臂 NOT_RUN（包缺陷），三态差分尚缺 DM/第三态

## 回执

DM 构建缺陷回 manager/architect（T670 树 build 配置）；SPF 基线原件已锁，
退休路径差分待 architect 对照 T597-fixed 同 scope（epoch4/resources=true）
与 SPF（WAIT_RETIRED/resources 未清）。
