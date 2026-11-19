# T632 developer 交付

2026-11-19。消费 architect T628 正式裁定及同步后的 T607 四表；仅开发、编译，不使用真机、不执行测试或 reader。

## 实现

- 首次错误采用同一个 atomic winner 封存 public error、op/scope/J/micro、invocation-local firstMathStatus 和 seal。root nonce 与实际 scalar 原字存于现有 conversionAndProbe 块；所有清除 firstError 的边界同步清除 firstMathStatus/seal，aggregateMathStatus 仅按原 root 初始化清除。后续试探不覆盖首故障坐标；fatal 失败不再清洁重置候选。
- 公共17不新增枚举：同根同故障17且非零 detail 为 NUMERIC_DOMAIN，17+0 为 NUMERIC_OVERFLOW；没有完整首故障观察则 MATH_DETAIL_NOT_OBSERVED。aggregate 仅观察，不冒充首因。冷档和 reader 交叉绑定实际 tail、permit、nonce、seal、坐标及 raw CTRL。
- softDiv 分母检查提前，0/0 记 bit1；0/非零保持零。保留实际参数、software binary128 和 rounded F32 的可观察位；未观察槽清零且标识。exp 总相对门仍为2^-20，转换判据预留2^-90软件近似预算，采用 (2^-20−2^-90)/(1+2^-90)。这是工程保守预算，不是已执行的全域精度证明；未观察实际操作数不冒充观测值。
- test-only PERIODIC_ALIAS_DUPLICATE→hook6，只允许 mode3 和周期边界。原 alias 同步后、FLOW_AUDIT 前破坏一个重复面，独立 U32 相等检查清 FIELD_RANGE audit bit1、报35；保存注入 scope/J/micro/面索引及 expected/actual。fatal 路径保留候选原字，禁止后续 enforce 抹平。生产二进制不含可启用的 hook6 注入分支。
- 不增加 GPU bank、dispatch、host GPU map/冷档预算；保留原16/820 dispatch 结构、公共错误码及正式阈值。研究路由默认 OFF，无渲染改动。新生成器消费 T628 补充表并检查同步身份；不改写旧 T608 包与报告。

内部落点、观察位、函数ID及推导来源见 developer-T632-wire-v1.json。新 mathSHA=1629bffe555eae801007af8ae7bc74e1a677b0b7a519853ff484a1f535d8a968；algorithmSHA=06b8881107a867dce309212532edadf823b679aafe0465ccf5933a2ab331395e。

## 编译交付

生产隔离目录 `.codex/local-test/t632-production`：最终 hvigor assembleHap **BUILD SUCCESSFUL，12.701s**。
负控隔离目录 `.codex/local-test/t632-hooks`：最终 hvigor assembleHap **BUILD SUCCESSFUL，12.711s**。
两套均完成 native、ArkTS 和未签名 HAP；逐文件 SHA 核对最终修改源与两套构建目录一致，实际 CMake 产出 manifest 的 testHooks 分别 false/true。

Khronos glslang16.6.0 完成生产/负控 compute 静态编译；Python py_compile、Node --check 通过。源码/合同/实际 ELF、静态库、HAP、manifest SHA 及日志路径见 `test/reports/t632/development.json`。

未运行 shader、reader、单测或真机；first-fault 实际触发、hook6 设备结果、数值精度和物理指标均为 NOT_MEASURED，交 tester/effector 后续独立执行。这些运行结果不作为开发卡关闭前置。
