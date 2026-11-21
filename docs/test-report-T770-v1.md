# T770 v1 · 目标 20 G13 门前隔离可见性探针真机验证——签包阻塞（如实）

日期：2026-11-21，effector。状态：**签包阻塞，按卡面纪律如实停**。

## 已完成（离线部分）

1. **T767 最终回据核验**：probe unsigned hap bae482dc 与回执一致，
   BuildId 88b370c7（合同指定值核对通过），bundleName=
   `com.example.helloworld.g13visibility`
2. **T769 判定器核验**：offline.json 836 resolver rows，
   fixtureOnly=true，deviceExecution=NOT_RUN，T757 控制清单 19 项在位
3. probe 树镜像到主树 .codex（签包工具链就绪）

## 阻塞点（精确）

**本机唯一签名 profile（entry-default.p7b）只授权
`com.example.helloworld`，而 T767 probe 包 bundleName=
`com.example.helloworld.g13visibility`**——sign.py 的 bundle 一致性断言
失败（这是正确行为：profile 与包必须同 bundle）。

已尝试：主树 sign.py / 镜像树 / T767 树 sign.py——同一 profile 同一失
配。机器上无 g13visibility 专用 profile。

## 所需前置

developer/architect 提供授权 `com.example.helloworld.g13visibility` 的
签名 profile（或把 probe 重打包到主 bundleName）。此后 T770 真机执行
只差一次 install+configure（采证管线其余部分全部就绪：固定 ordinal
reset/compare、WORK/跨 lane、同代 tail map、PACK 顶点消费、非法入口、
退役原件的采集路径均已按 T769 判定器挂接）。

## 原件

`test/reports/t491/t760/v1-t770-sign-blocked.json`
