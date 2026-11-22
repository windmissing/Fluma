# T496/T724/T725/T726 v1 · PC 离线准备批量登记

日期：2026-11-22，tester，Windows PC。
台账：`test/reports/t825/t496-724-725-726-prep-ledger.json`。

## T496（GPU 同源外线程 SOURCE 与异步采集对照）

- 最新 v7：glDispatchCompute x=1 **硬崩溃精确原件在档**（崩溃点/callId/
  functionId/wallNs/gltrace 全序列）
- D1/D2 差分与 32cycle NOT_MEASURED（未到正控非空 SOURCE 前置）
- PC 状态：fixture 域就绪（崩溃原件归档），create-only reader 链 v7 可用

## T724（M3/G9 同次请求与期限判定离线准备）

- 判据源：design-T497-gpu-M3-{fixtures,checks,manifest}-v1 全在
- PC 状态：**offline replay 就绪**（fixtures+checks+manifest 齐备）

## T725（来源链与同窗负控离线回放）

- 与 T724 同 fixture 族（offline replay）
- PC 状态：**就绪**

## T726（四臂首帧回执离线对照设施）

- 四臂 prefix 设施已在（t804/t809 x-*、t818 y-* 原件）
- PC 状态：**对照设施就绪**

## 如实边界

四卡均为**离线准备域**——设施与 fixture 已验证在位；真机 GPU 原件不属
PC 范围（与 T825 结论一致）。
