# T707 RX 修订产物身份与签包

2026-11-20，离线交付 T645。使用 T699 实际构建的 unsigned HAP，SHA-256 为 `cce353bc9984c0b151e271b2cbabc2a41c258759abcb31e990c0dd969879c5a1`。原样签名并通过 verifyApp，逐个校对 14 个原包条目的数据摘要。

重新运行 glslang 的 compute 检查，退出 0。规范源 145492 字节，SHA-256 `5d9cd082f30983806537c52cf2fb1aa96ca870eb50b7b0d59a84e77fbcf2881c`，与生成头、实际链接 ELF、unsigned 和 signed HAP 中的 RX ELF 字节吻合。包内 AArch64 ELF BuildId 为 `624ba44500d160259727e2b25a8422ebe5ee1884`。

- 签包 `.codex/local-test/t257-sign/T707/T707-signed.hap`，SHA-256 `dfc7f60ebaf4c9889452dd4d423f9f977a16191f59c8298edab421df6330e962`。
- 原件核对见 [identity.json](../test/reports/t707/identity.json)、[handoff.json](../test/reports/t707/handoff.json) 与 [签名回执](../test/reports/t257/T707.json)。
- T674/T687 旧规范源 145458 字节、SHA `4faf48aa…`，保留为原基准。T645 设备装载、驱动编译及后续物理验证均为 NOT_RUN。
