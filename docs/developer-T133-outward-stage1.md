# T133 向外算术与局部证书：阶段一开发回执

日期：2026-10-26。角色：developer。不使用真机。

## 实现

- `P011ExactDyadic.compareAffineBound` 使用固定 272×16 位槽验证至多一个乘积加七个 binary64 标量；拒绝超项数和非有限表达式。返回共享结果，调用方在再次比较前复制 `code/sign`。
- `P011OutwardArithmetic` 实现二进制相邻值、加减乘除定向端点、区间四角及保守半 ULP 上界；数值越域和零除返回显式错误。
- `P011WriterEnvelope` 提供定容 node/branch SoA 和容量拒绝。`P011LocalPatchVerifier` 只在 LOCAL 检查 changed 列及相邻边，入口已越界时采用残差不恶化约束；FINAL 扫全部列及边。证书标记 `materialOnly=true`，不声称已验证库存外的预算、pool 或审计。
- `P011NoWritePatch` 提供逐位浮点/ID 不变校验，必要接触冻结返回 `CONTACT_HOLD_UNSUPPORTED`。此阶段未接入生产 strict 路由，未实现后续 writer 影子事务。

## 离线验证

- `node test/run_t133_outward_units.cjs`：PASS，独立 BigInt dyadic 参照覆盖方向端点、区间、affine、LOCAL/FINAL、NoWrite。
- `node test/run_t112_exact_dyadic_units.cjs`：17/17 PASS。
- `node test/p125_dyadic_oracle_v2.cjs`：14/14 PASS。
- hvigor `BuildNativeWithNinja`、`CompileArkTS`、`PackageHap` 通过。`assembleHap` 最后仍在既有 `SignHap` 配置处失败：`build-profile.json5` 的签名密码短于 32 位；不涉及本次源代码编译，未改签名配置。日志见 `test/reports/t133_build_wip.txt`。
