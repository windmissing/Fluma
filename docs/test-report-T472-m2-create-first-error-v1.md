# T472 · GPU M2 创建首错真机诊断（v1）

- 日期：2026-11-11
- 角色：tester（真机 62T0225B10010020）
- 结论：**错误 6 来源已确认并有原始证据**；核验范围覆盖卡面要求（原始 program/阶段/compileStatus/log/queryError、context 能力）。新空场二次冷创建复现同错（确定性好）。
- 依据：T475 交付包（沿 T468→T452 生成链，含 T468 诊断按钮；T468 专用签包未单独到件，本卡使用其生成后继 T475 包完成同等取证）。
- 证据：`test/reports/t477/device-cold/`（与 T477 同一设备窗口，原件共用）

## 错误 6 来源判定（卡面核心问题）

**TRANSPORT_CREATE #6 = SHADER_FAILED = 驱动 shader 编译拒绝**：

| 证据 | 内容 |
| --- | --- |
| 诊断导出原件 | `t468-create-1-1794372248891-0.log`：`0:1138: S0001: Function call discards 'coherent' access qualifier.`（Maleoon 驱动在 GLSL 1138 行拒绝 coherent 限定符函数调用） |
| 收据 program 级 | role=SOURCE、stage=COMPILE_OR_LINK_FAILED、compileStatus 失败、shaderLogReported=68/copied=67、logReported=0（program log 空——失败发生在 shader 编译阶段未到 link） |
| queryError | cacheRawGL=0、originalRawGL=0（无遗留产品 GL 错误——失败为编译结果非状态污染）、cacheRawEGL=0x3000（EGL_SUCCESS） |
| context 能力 | supportedFormats=[36705]（真机 binary format 非零——**格式能力门通过，排除格式缺失归因**）；environmentSHA=5ef5dd69…（驱动/上下文指纹稳定） |
| 可复现性 | 两次冷创建（清数据首启 + 保留数据重启）同错同日志，确定性失败 |
| 空 log 证明 | log1 文件 0 字节（第二 program 从未尝试）+ 收据 logBytesHex 空——API 真实调用（COMPILE_ENTER 时间戳在），失败路径无假 log |

## 归因分流（回 architect，不直接拆 kernel）

1. 根因 = 驱动 GLSL 语义校验：`coherent` 限定符在函数调用参数传递中被丢弃（GLSL ES 3.1 规范允许驱动报错）。
2. 修复方向（developer）：shader 源 1138 行附近移除 coherent 限定符或以宏条件化（真机关闭/软件 GL 保留）。影响面：该 shader 文件的三 program 全部（SOURCE 首失败即停，INTERMEDIATE/FINAL 未达）。
3. 该证据同时闭环 T461（SHADER_FAILED 根因）、T468（诊断通道有效性）、T477（缓存主链前置）。

## NOT_RUN 如实

- 原 T461 功能项（能力探针明细/SOURCE13/QUANTUM/merge+birth/短接受/同token/回滚/冷档 reader/耗时fps）：依赖 Create 成功，全部未行使——不能因诊断到件冒运行通过（卡面红线遵守）。
- 新空场二次创建在 STATE_LOST WAIT_RETIRED 挂起后未再单独驱动（第一次复现已两次，确定性已足）。
