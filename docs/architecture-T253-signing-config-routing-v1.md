# T253 隔离包签名失败首次归因与可部署交接

2026-10-29，architect；原卡 `6ae2a418e4b0c1a6bd6b2821`。不使用真机，不签包，不输出/修改密码、私钥或设备设置。核对 T246/T248/T250 正式报告、各自 `test/reports/*/sign.txt`、生成器与根/隔离工程签名配置的**字段名、长度及文件存在性**。

## 归因

三份日志均为 `SignHap 00303116 Configuration Error`，定位各自 `build-profile.json5` 的 `storePassword or keyPassword field ... less than 32`。T248 生成器从根工程原样复制 `build-profile.json5`；T246 复用 T248 复制器；T250 从 T212 隔离工程生成，所得签名 material 与根工程一致。四份配置的两个字段长度均为 **6**，material 中 storeFile/certpath/profile 所指文件均存在。实际密码值未打印、未验证其正确性，密钥内容也未核；本轮能证明的是 **hvigor 当前签名配置的长度门不满足**，不能推为密钥损坏或开发编译失败。三个工程的 CompileArkTS/native/PackageHap 已通过，各有 unsigned HAP 与 SHA，SignHap 失败不应反向阻止开发交付登记。

## 可复用授权路径与操作交接

仓库早期隔离包已有通过 `hap-sign-tool.jar` 本地签名并安装真机的归档流程，见 `.codex/local-test/p011-hot-verification/test/artifacts/p011-v28-dual-input-20261018/DELIVERY.md`；此只说明**存在既有授权流程**，不证明这些新 unsigned HAP 已签成或可安装。由具备原授权签名材料的签包执行者，在隔离输出副本上复用该流程，先核 tool/JAVA 版本、证书/profile/keystore 对应同一 applicationId，再分别对 T246/T248/T250 的 unsigned HAP 签名；保存输入 SHA、签名工具退出码/无敏感值日志、输出 SHA、签名身份校验及可部署包路径。不得把任意字符串填充到 32 字符骗过 hvigor；不得复制旧签包当新构建；不得在报告中泄漏凭据。若现有授权流程实际不可用，精确缺项为**可用于这些 applicationId 的有效签名材料及其授权使用方式**，由材料持有人提供/执行；不要求把明文密码回填仓库。

T247 真机窗口探针、T249 V2 观察、T251 同包事务设备计量各自以**对应新 unsigned SHA 的有效签包**为部署前置。T246/T248/T250 已编译的部分工程交付可独立关闭或按实质未实现项继续，不因签名再设开发完成门。签包后设备测试仍须核自身产品接口/证据范围，不能把可安装等同产品 PASS。T253 工程分流完成；签包实际执行/设备部署未做。
