# T157 GPU 信封与 shader 正式修复

2026-10-27；developer；不使用真机；按 T152 §2/3/5/6 第一步。

正式 reduce 仅修 R6/R7：分别读取 h50..53、h54..55+DONE/0。R0..5 与严格 owner 验证保持原字节。新增命名 wire schema/生成校验，CPU_EMPTY 的同布局编码显式命名。build/query 与 T142 归档 whole-word 改名副本逐字节一致：half 属于保留字，其余 length/start/stop 仅保守避名，未声称它们各自是保留字。旧 shader 和失败报告未覆盖。

独立预期按 T138 明写完整32words，不导入生成 schema 自证：PAIR/NO_PAIR、高word非零、每个result word翻转、owner/config/run/attempt/取消等失效及 n=0/1 CPU_EMPTY 均通过。另直接解释正式 GLSL 输出表达式，覆盖原 SHA 错位；这项是 SOURCE_EXPRESSION_EVALUATION，不是 GPU 执行。

executor 保存第一次读取的 GL 原码，fail 不再二次 glGetError；保留 wait 数值/字符串及 stage，编译/link info log 取完整报告长度。driver 分列 execStatus / consumeStatus / oracleVerdict，协议错误不再冒 ORACLE_MISMATCH。

编译：MSVC /fp:strict 独立布局与原范围/owner 驱动通过；OpenHarmony aarch64 so 编译通过（build.txt 的 BUILD_OK）。源码表达式4项、独立C++全部断言、192/512 CPU范围与owner检查通过。宿主实际返回 GLES_31_REQUIRED：shader compile/dispatch/真实消费为 NOT_RUN，不报设备通过。证据与 ELF/driver/源码SHA在 test/reports/t157/，入口 test/p013_compute/build_t157.cmd。

不改生产路由、原物理算术或 Canvas 路径；所有场景仅新 XComponent+shader，失败 FAULT/显式 STALE。T159 接下游 env/Worker/完整身份/打包，T161 另做真实消费及故障/完整事务收益。
