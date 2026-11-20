# T600 v2 · G11 组合研究真机首测——Worker 资源缺包阻塞创建链

日期：2026-11-20，effector（tester 兼），真机（011124264R000337）。
被测：T687-g11-signed.hap（T687 关闭交付=T639-delivery-v2 基+G11 独立
parser/签包，signedSha e237f6b6，sign=PASS）。
证据：`test/reports/t491/t600b/`（l1–l4 布局、h600.txt zip 一致性原文、
v2-g11.json）。

## 入口与页面（设施验证成立）

首页直达「**G11 · 接受通量组合研究**」——档位 R1/K192/V2048/ROOT 徽标+
配置/封存 seed/停止/测试源/倾斜/seed 原件/冷档按钮全在（W01–W12 设施的
UI 域就位）。

## 执行与阻塞

配置（R1/K192/ROOT）→等 40s：状态恒 **STOPPED「等待有效新管线帧」**；
冷档按钮无产物（files 目录空）。

**根因（hilog 原件）**：
```
zip_file.cpp(IsEntryDataConsistent:1036) GetEntry failed for ets/workers/G11Cold
extractor.cpp GetSafeData: Entry data not consistent for ets/workers/G11ColdWorker.abc
```
zipfile 清单核验：签包内**无 ets/workers/\* 条目**——G11ColdWorker.abc
未打包。

## 判定

- T687-g11 签包**构建缺项**：T639 树 workers 资源未进 HAP——Worker 加载
  失败→G11 冷链卡在创建前
- G11 臂全部（W01–W12 真机执行）阻塞于此构建缺陷——回 tester/developer
  修 T639 build 资源打包配置
- 不宣物理/像素 PASS（如实 NOT_RUN）

## 回执

G11 设施 UI 域验证成立；构建缺项回 tester（T639 树打包配置）——修复后
W01–W12 每轮即测。
