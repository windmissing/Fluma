# T592 v2 · 单沙预约 HOLD 续证——prepare 崩溃根因（产品 JS TypeError）

日期：2026-11-19，tester，真机（011124264R000337）。
被测：T592-rev2-signed.hap（t598-final 树产品码未变，inputSha ba8980ef→signed
eeb39a4f，sign=PASS），bm clean 后冷启直达 P051DerivedV2Demo。
证据：`test/reports/t491/t592b/`（jscrash-160328.txt 原文 + v2-jscrash.json +
布局/截屏原件）。

## 执行链

1. 固定键盘确认按钮：**"已确认固定键盘" ✓**（响应式合同成立）
2. "收键盘并准备"点击 → **应用进程直接消失（被踢回 launcher）**
3. faultlogger：`jscrash-com.example.helloworld-…-20261119160328166.log`

## 根因（产品代码缺陷，非观察窗口问题）

```
Reason: TypeError: same is not callable, same is undefined
栈：noteChange        PrepareReadinessV4.ets:18:97
    changedEntry      PrepareReadinessV4.ets:19:58
    anonymous         P051DerivedV2Demo.ets:130:102
```

缺陷码（PrepareReadinessV4.ets:17-19）：
```ts
this.ownerV4 = JSON.parse(JSON.stringify(o)) as PrepareAttemptOwner;  // 17
noteChange(...){ if(!before.same(after)) ... }                        // 18
```
`ownerV4` 存的是 **JSON 纯对象副本**（无原型方法），随后 `before.same(after)`
调用该方法必然 TypeError → 应用退出。prepare-observe 路径每次点击必崩。

## v1 勘误

v1 报告的 `PREPARE_OBSERVATION_FAILED`（窗口读回失败）是下游症状——真实根因是
prepare 路径 JS 崩溃导致应用退出，观察窗口自然失效。v1 两轮"复现"实际是本崩溃
的两轮触发。

## 判定与回执

- **预约 HOLD 链 NOT_REACHED**（prepare 崩溃在前，reserve/RESERVED/HOLD 不可达）
- **回 developer**：PrepareReadinessV4 owner 快照不能调用 JSON 副本的方法——
  比较应字段级进行，或保留原型对象/用结构化克隆保方法。修复后本卡完整续测
  （固定键盘确认/prepare 观察/预约 PNG 严格标定全流程脚本就绪）
- jscrash 原文与两轮布局/截屏原件归档
