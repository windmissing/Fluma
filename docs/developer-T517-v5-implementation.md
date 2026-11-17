# T517 开发记录

2026-11-16，developer。用户本轮裁定仅开发、编译、提交，不执行测试；不使用真机。当前为局部开发交付，**不是完整 T517 完成**。

独立 `gpu_resident/m3b`，旧 M1/M2/V4 默认和阈值保持。V5 研究页必须显式选择新空 run；起沙 OFF 为默认，合成研究标 sourceKind2。候选配置包含全部参数、revision 与 SHA，未将模型参数或旧加速度残差认作真实 sensor 资格。

## 已落代码

- T512 同源字段与四档七域扩展，原 header 固定位置，扩展 WORK spare 后重定位来源 ledger/index/origin 等后缀；五角色固定 V5 pass 表，SOURCE72 与 1024B 接受尾。
- SOURCE birth/merge 的证书、循环标签、U 缓存写入切点；完整扩展状态参与 trial/SPARE 复制、回滚及逐字比较。
- LAND 顶层容量预检、来源追加/合并及比例返还标签；三色床流按 receiver 可表达前缀限量，单 64-lane group 逐片同步和首错归约；lift 取真实顶层、最低空槽/新 ID/generation、预算/guard、反重力整数 RNE 出生。
- 附属流 planned/sealed 共同游标、ENERGY/REQUEST/来源事件；SPARE 在物理切点重建来源与证书，selected/authority/consistency 比较新增状态和附属流。
- input64 冻结到 plan、每 root 再核新鲜度，失效撤权；resume 新输入代/session。sensor 时域尚无设备语义证书，明确拒绝，不由页面自授资格。
- 独立 native bridge、同 token 活包 medium 绘制、XComponent/shader 研究页；无 Canvas/软件 GL 产品回退。
- T527 四档 ROOT/FULL 分段容量、整行有界分页、两 slot 生命周期适配、锁外 detached block 保留计账直到实际 free 回执。

## 具体未完成项与合同问题

完整 ROOT/FULL 捕获、FULL plan 预约及五个 BASE/SOURCE_CONTEXT 锚点保留、完整冷 collector/reader 和诊断 V5 adapter **尚未交付**，不能将容量表和生命周期 helper 当作完整 cold 实现。当前 cold API 在分配前返回 `PREFIX_DIGEST_POLICY_REQUIRED`，capability `supported=false`；FULL 模式配置明确拒绝。跨旧 M2/V4 和 V5 的同 App 统一 slot 信用接线亦未放行，不抬高旧 M2 限额。

原件：`architecture-T527-v5-cold-manifest-v1.json` 的 ROOT/FULL 第 0 段 `prefix=[0,32768)` 包含 `descriptors64x32=[1024,9216)`；每 descriptor 的 23..30 是被描述段的 SHA256。若 prefix 自身也有 descriptor，完整 prefix 摘要包含自己的摘要字节。现有合同没有定义 canonical-zero、排除字段或 prefix 不入 descriptor 的规则。

已读取正式 MD/机器表并按字段核实；已在原 T517 卡评论请求 architect 明确摘要输入及封存/reader 顺序。没有选择一个私有摘要规则生成貌似 COMPLETE 的档案。接手 architect 明确该规则并同步机器表后，developer 继续上述完整冷实现。其它已写代码仍须独立 tester 检验；编译不等于物理或协议通过。

## 构建与交接

`node test/t517/generate.cjs .codex/local-test/t517-delivery` 生成包含 M2、V4、独立 V5 的隔离工程，保留产品默认。生成器复制工作区权威源码；`generate_native.py` 等为本次初始迁移工具，不能单独运行后覆盖后续手工接线。shader 生成入口 `assemble_quantum.py`，ABI 生成入口 `generate_contract.py` / `generate_cold.py`。

用户职责裁定前曾运行局部 helper/SOURCE 夹具，不能据此宣称完整新版本测试通过；裁定后仅编译，不再执行测试。测试资产保留给 tester，不作为开发关卡。

完整隔离工程构建：`assembleHap --no-daemon` **BUILD SUCCESSFUL**，34 tasks，46.577s，零编译错误。日志 `.codex/local-test/t517-delivery-build.txt`。产物标记 development/partial，不作为完整 V5 验证包。

签名构建收据 `test/reports/t257/T517-development.json`；signed HAP SHA256 `a9e328a38a865801467d5beb779d7adfd2a84368b77783c898184d9bc2115b8a`。签包验证是构建步骤，不含应用测试或设备安装。

## 2026-11-17 T535 接续

T535已解除prefix自身SHA输入歧义，新增PrefixDigest固定word279..286虚拟归零与HexDigest字节序helper；旧捕获仍未启用，不能把helper当完整V5_2档案。隔离工程t517-t535-digest native/ArkTS/HAP编译通过，34tasks/50.614s，未执行测试或真机。

发现独立wire合同缺项：T527机器表仅列descriptor字段名，非prefix各段kind枚举、segmentRevision数值、ROOT rootIndex/role/trial及NOT_EXECUTED行计数规则未绑定到各段；T535只规定prefix(kind1/rev2/身份0)，其余称沿T527。现有helper kind=index+1是实现假设，不能自行晋升正式合同。已原卡给architect提交精确补表请求；需要机器表逐段值或明确确认index+1/revision规则后继续完整writer/reader。

未完成仍为ROOT/FULL捕获、五BASE/SOURCE_CONTEXT、完整collector/reader与跨route信用。已查T527 MD/JSON、T535 MD/JSON与现有Validate，未发现非prefix数值定义；严格实现需要对端同一解释，developer不私定wire。本人行看评论；并继续用户另两卡。原PREFIX_DIGEST_POLICY_REQUIRED回执暂保历史disabled状态，不将其当新阻断原因或完整功能。
