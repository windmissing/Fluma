# T375 M1 首次真实提交预热

M1 每次成功建立 transport 时，在原有零工作预检之后、READY 和首 anchor 之前，由同一个 GL owner 运行生产 quantum shader 的 pass 0 的一个工作组并 flush。显式绑定该 pass 所需的 uniform 范围。只写 candidate 和 workspace。正式 root 以原 pass 0 全量初始化；预热没有 root nonce、publication、事件、fence、lease 或热提交计数。预热发生 GL 错误时返回 TRANSPORT_CREATE 故障，READY 不开放。

## T376 读数

- 沿已有 M1 启动参数 `grsm1=true` 进入，签包 SHA 见同目录 `sign.json`。包未更改启动路由。
- `T324_STAGE42_V1` 保留 1–41。每次进入预热的 run 发两条 42（SubmitWarmup），两条都用 `root="0"`。第一条 `carry=0, delta="0"`。第二条 `carry=1`，`delta` 是原始 steady wall ns，`code` 是 GL 状态。匹配同 run、epoch、surface、TID 后比对 READY 和 AnchorSet。预热前准备失败的 run 没有 42。
- M1 status 追加 `warmupWallNs`，与当前 run 的 42 exit 相同。同步修正了 M1 status 的 `submitPumpWallNs` 后多余引号，使追加字段可被 JSON 冷出口读取。
- 原 GAP 窗和分批 flush 布局维持。用热路径首个 SubmitEnter/SubmitExit 对照 8 ms，逐批结果与预热冷窗分列。双源同 token 上屏、设备时延和冷环设备完整性由 T376 取证。

## 离线结果

- 脚本化提交循环 12/12，桥接 11/11，冷导出 24/24，冷导出容量与字典防漂移 4/4。
- 宿主 GLES App 上同 TID 的 stage 42 成对发生在 READY/AnchorSet 之前。状态 JSON 与退出行披露相同的预热耗时。该宿主进程实测冷预热约 43.66 秒，结果反映 PC pbuffer 驱动路径，设备收益和冷启动时延交 T376 实测。
- Hvigor 编译成功，签名和验签成功。签包 .codex/local-test/t257-sign/T375/T375-signed.hap，SHA256 b63d6ec9843cee97d352259f8ed886d377ad2914cbbd23c58e70f94212eb0fe1。

- 真实 GLES 宿主 partial suite 在 192×64 和 512×128 各通过 35/35。两档均验证真实 pass 0、保护缓冲哨兵、首根位型、失败后重建及 CASE_F 分批 flush 计数。
