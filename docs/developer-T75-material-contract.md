# T75 G2 补充合同实现

日期：2026-10-26。developer；不使用真机。依据 T67/T68，实现三项原 BLOCKED_CONTRACT，未重开 T54。

- 显式 `bindMaterialConfiguration` 接收独立 session/revision/accepted-configuration digest。相同绑定幂等，同 revision 异 digest→402；重绑定不删除 active/pending。stage 捕获绑定身份；READY 旧目标不可激活并返回402。receipt 加 requested/bound revision、bindingId/digest，未从 desc 反填期望值。
- session 内 generation 高水位与驻留完整 fingerprint。固定顺序长度编码覆盖全部通过校验的不可变 metadata、实际字节 SHA、生成参数；参数缺失和字符串有不同类型标记，参数 UTF8 上限16383字节（超限拒绝，不截断）。同代冲突→400/identity，完全重 stage 保持状态与上传进度，退役代禁止复用；受控 Restore 保留原身份与 CPU 数据，清除纹理目标并重传。
- 非法预算优先返回400/byteBudget/零上传，不触发 GL/epoch discard、不推进 pump/cursor；budget1 合法且可零上传。controller 透传结构化收据。失败直接抛出明确 NEWPIPE_FAULT_NO_CANVAS，不执行历史回退 callbacks。

验证：`test/run_t75_material_bridge.cmd` 编译并运行真实 core + NAPI bridge（合成 GPU/NAPI 边界），覆盖旧生命周期、字节范围/实际哈希/alpha及新增独立目标、同代冲突/幂等、重绑、恢复、非法预算和后续合法上传。`run_t75_material_core.cmd` 是同一合并执行器入口。独立 controller 3/0。证据 `test/reports/t75/native.txt`、`controller.json`、`build.txt`。

ArkTS CompileArkTS 与 BuildNativeWithNinja 成功；旧 CONTRACT_PENDING/Canvas 测试和报告保留历史身份，未覆盖或降低判线。此为隔离 G2 API；完整产品配置协调器仍须从已接受配置计算确定性 digest 并显式绑定，真实 GPU/全场景/显示验证不由本卡宣称通过。
