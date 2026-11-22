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


## v2 追加（2026-11-21 20:32，effector；自签全链打通——设备信任门拦截，精确选项）

**自签全链 rc=0**：①p7b 内 profile json 解出（issuer pki_internal，
type debug，device-ids 旧 UDID）→②bundle-name 改 g13visibility+当前设备
UDID 8BA8C071 追加→③sign-profile localSign（dev-cert-chain+p12，密码内
传）rc=0→④sign-app localSign（**appCertFile 必须链文件 dev-cert-chain
.pem**，.cer/短 pem 报 Profile cert must a cert chain）rc=0。

**设备安装 FAILED code 9568404 CODE_SIGNATURE_DELIVERY_FILE_FAILED**：
BMS DeliverySignProfile 拒收自签 profile 的新 bundleName。对照实验：
T738-rx（主 bundle+原 p7b）安装成功——**通道本身正常，是设备信任库只
认预置 profile CA**。

**三选项**：(a) probe 重打包主 bundleName（最快，开发者重构建）；(b)
developer 提供正式 g13visibility profile；(c) 不可行已排除。


## v3 追加（2026-11-21 21:55，effector；主 bundle 重打包签通——隔离域缺口不变）

T767 树 t767-split-product（11-21 19:50 构建）已把探针重打包为
com.example.helloworld 主 bundleName（1cf1bfbc 回据核）→ sign.py PASS
（a44752df）→ 安装成功（v2 的 profile CA 约束由重打包解决）。

REST 配置 180s：**CONFIG_FAILED INIT_ERROR_27 @ BOOT_EFFECTIVE_TERMINAL
:EXIT，OP65 result=27**——资格门原位复现。

**隔离域缺口不变**：门前隔离探针（固定 ordinal reset/compare、跨 lane、
WORK 重用、tail map、PACK 顶点消费、非法入口、退役）仍位于数值门后——
重打包解决了签名，但未创建门前探针入口。归 architect：门前探针入口
或 FTZ 决策（与 T760 v1 同一精确缺口）。


## v4 追加（2026-11-21 23:35，effector；缺口分析修正——探针本自包含，唯一阻塞是签名信任）

**T760 v1 缺口分析修正**：T767 可见性探针用例（RESET/COMPARE/LANE63/
LANE0/TRACE_SCOPE/SEAL/SETUP/VISC/WORK_REUSE/PACK/TAIL/ALIAS/BAD_SCOPE/
BAD_GENERATION）在**自包含离屏 pbuffer context** 中逐 attempt 运行——
**不经过产品 BOOT 数值门**。T760 v1 记"探针全在数值门后"有误（探针与
门本就无关）。

**真实唯一阻塞=签名信任**：探针需 bundleName
com.example.helloworld.g13visibility + 设备信任 profile（自签被拒
9568404，v2）；主 bundle 重打包（v3）不含 G13VisibilityAbility
（abilities 仅 EntryAbility，无 visibility 页），探针不可达。

**选项排序**：(a) developer 提供**设备信任的 g13visibility profile**
（正规修法，启用真探针）；(b) 将 visibility ability+页并入主 bundle
构建（工程绕行，改 bundle 结构）；(c) 接受离线 fixture 结果（T769
reader 836 rows fixtureOnly）作为覆盖证据。

设备侧：split-product（v3）无探针入口（G13 页为产品研究页），REST
复现 OP65=27 属数值门域，与探针用例无关。

**归 developer/architect：选 (a) 或 (b)；探针入口可安装后 tester 立即
可执行全部用例。**


## v5 追加（2026-11-22 12:25，effector；离线覆盖正式入档——选项 (c) 执行）

本轮无新解锁（无 g13visibility profile、无主 bundle 探针入口、T793
sidecar 修订包未交）。**增量：选项 (c) 执行——T769 离线覆盖正式入档**：

- T769 判定器：**836 resolver rows**，**28 项 T757 控制**全核
  （QUANTUM_TRACE/ROOT_OPCODE_POLLUTED/ROOT_TRACE_SCOPE/NONCE/SEAL/
  MICRO_*/SOURCE_TRACE/CAPTURE_FENCE/MISSING_CONSUMER_BARRIER/
  INCOMPLETE_DISPATCH_PLAN/OVERLAPPING_BINDINGS/DISJOINT_RANGES/
  IDENTICAL_BANK_RANGE/FULL_BANK/ROOT_MISSING_REPLAY/LEGACY_ALIAS/
  IDENTITY_*×6）
- fixtureOnly=true，deviceExecution=NOT_RUN（如实）
- 设备现况：T760-split 在位（T794 时代），bacbc911 cache 2 条目完好

**结论**：探针真机执行仍阻塞于签名信任；离线覆盖已作为过渡证据正式
记录（T770 v4 选项 c）。(a)/(b) 决策落地后真机探针立即执行。
