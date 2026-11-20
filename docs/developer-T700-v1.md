# T700 构建回执当轮调用来源绑定

以新的显式调用清单绑定 configure、compdb、实际 Ninja 编译/链接、重配、无工作、仅 RSP 重链和复跑。每步保留独立 stdout/stderr、argv、cwd、退出码及前后输出 SHA。链接器子回执与其父步骤 UUID、原始日志和 ELF 匹配。缺少证据或输出变化时拒绝发出成功回执。静态模式只报告当前制品检查。

开发编译使用带空格的全新构建目录。Ninja 实际生成 broker 和 consumer 两个 ELF，重配后无工作，单独触碰 RSP 后重新链接，再次无工作。详细证据在 .codex/local-test/t700-receipt，摘要在 test/reports/t700/compile.json。未运行独立测试、产品或设备。
