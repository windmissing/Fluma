# T710 G11 Configure 首错与 Worker 入口回执

现有 g11Status 增加最近 8 笔 Configure 尝试及生命周期回执。合法请求入队即返回编号。按编号保留同一笔入队原返回、owner 进展、已绑定 scope、EGL/loaded module/Controller 返回、bootstrap 原返回，以及独立的首错、退役和清理结果。晚返回按编号归档，lastReply 仍报告最近命令。页面分栏保留配置原回包和 typed view 规格、controller/presented 摘要、生命周期和归档状态。页面刷新只关联当前编号与已绑定 scope。

独立 Worker 探针只发 HELLO 并等待同编号 ENTRY_ACK。真实归档复用同一握手，在入口确认后发送 BEGIN。BEGIN 到达、原 native 拒绝、文件封存及异常、退出、取消分别记录。启动等待上限 5 秒。归档开始后沿用原 CANCEL、End 和 60 秒冷资源界。未取得有效 traceId 时保留原拒绝，且不会误发 End。Worker URL、模块名与 esmodule 打包沿用原路径。

hvigorw 原生、ArkTS 与 unsigned HAP 通过。反汇编确认实际包内 modules.abc 含可执行 Worker 入口，安装 onmessage，且 ENTRY_ACK 分支在 BEGIN 和归档调用之前。R1 生成 GLSL 与 T639 字节一致，DM 组合继承前序 T706 的捕获作用域修订。首轮因 ArkTS 临时 .tsbuildinfo 路径 ENOENT 中止，隔离目录复编成功。产物在 .codex/local-test/t710，回执在 test/reports/t710/compile.json。交 T600 验证同 scope Configure 及实际 Worker 启动，resolver 结果待设备证据。
