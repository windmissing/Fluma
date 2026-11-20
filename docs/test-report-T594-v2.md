# T594 v2 · V5 冷档 RED 基线——T674 修复旧错，新错 coherent qualifier

日期：2026-11-20，effector（tester 兼），真机（011124264R000337）。
被测：T594-rx-signed.hap（t674-delivery-v2=T674 修复树，inputSha 3d4042bc→
signed 4efb9d63，sign=PASS）。
证据：`test/reports/t491/t594b/`（l1/l2 布局、h594.txt driver 原文、
v2-red.json）。

## 执行链

| 阶段 | 实测 |
|---|---|
| LOAD | LOADED_FILE_OBSERVED：新 so sha256=72d3b721 / 1404632B / BuildId e989ed10（T674 产物已进包） |
| ENABLE | configure 进入 GL 线程 |
| **SHADER** | **CONFIGURE_FAILED_8**——driver 原文 `0:1063: S0001: Function call discards 'coherent' access qualifier` |

## 判定

- **T674 的 slot redeclared 修复生效**（旧错误 S0022 消失）
- 修复暴露**下一层 shader 语义错误**：生成 shader 第 1063 行函数调用丢弃
  coherent 访问限定符——回 developer（T674 后续：coherent 限定符修复）
- V5 ROOT/FULL 冷档链仍在 configure 门前——**RED 基线**（NOT_REACHED 如实）：
  冷档/READY/pulse/触摸均不可达

## 修复链状态

slot redeclared（T674 修）→ coherent qualifier（待修）→ configure PASS →
T594 可达。设施就绪，每轮修复即重测。
