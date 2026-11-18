# T579 v1 · GPU 修复签包与独立执行设施完整交接

日期：2026-11-18，tester，不使用真机/不安装（签名与执行准备域）。
依据：滴答 T579 prompt（复用 t151/t257 流程；不写产品代码；修改测试脚本/夹具属 tester）。
证据：`test/reports/t579/delivery.json`（五包+差分 exe 全 SHA 与交接矩阵）。

## 五类签包（全部 sign=PASS verify=PASS，test/t257/sign.py 授权流程）

| 包 | 来源树（unsigned 已交） | 修复内容 | 对应执行原卡 | HAP SHA256（前 16） |
|---|---|---|---|---|
| T579-m2fix | t562-m2-fix | T551/T562 空日志导出兼容修复 | T461 续测 | a332c0a1cd541e58 |
| T579-m3diag | t562-m3-diagnostic | T550/T562 M3 首拒诊断 | T545 续测 | 6d346433f20571bd |
| T579-g9diag | t562-g9-diagnostic | T550/T562 G9 诊断+安全启动 | T542 续测 | c999825ce467f445 |
| T579-stage | t564-stage-tree | T564 分层资格/整数属性 | T560 域续测 | 819dc2dc28173019 |
| T579-lab | t565-lab-tree | T565 lab 排空终态/分页导出 | T565 域续测 | c772f665e77d0448 |

每包字节 8.8–10.8MB；HAP 全文 SHA256 与包内 ELF（libgrsm1.so）SHA256 在
`test/reports/t579/delivery.json` 逐包登记。

## 独立差分可执行程序

- `g9_differential`（T549-frozen，git e429ea7）：3,640,304B，aarch64-linux-ohos ELF
  SHA256 前缀 b62730f3becef…；用法 `g9_differential INPUT_DIRECTORY NEW_OUTPUT_DIRECTORY`
- 设备执行协议与输入五件套：见 test-report-T541-device-predicate-v2.md（已打通）
- T541/T569 差分续测直接复用（设备谓词 fail@vertexSSBO=0 待 T560 裁决后重评）

## 交接边界

- 本卡不安装、不用机：真机执行在各原卡串行（T461/T545/T542/T526 已可各自取包续测，
  不以本卡集中交接为新门）
- 未写/未改任何产品代码；五棵来源树均为 developer 交付的 unsigned 构建
- 无缺失实现：五棵树全部含完整 unsigned.hap（构建日志见各树 entry/build）

## 签名材料与流程

test/t257/sign.py：内存读凭据、verify-profile→bundle 匹配→证书公钥一致性→
CertPublic.java 双端校验→sign-app→包级 verify PASS。签名报告在 test/reports/t257/
（T579-*.json 凭据脱敏）。
