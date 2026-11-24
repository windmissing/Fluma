# T922 · 分版工件和封存 SOURCE 离线判据

2026-11-24。独立从 PE32+ 节表定位 owner 角色记录，逐个核实磁盘 SPIR-V 长度、magic、SHA 和角色清单。对照未使用 BOOT 自报来给旧包结论。

| 封存包 | DM2 | R1 |
| --- | --- | --- |
| baseline | 角色一致 | 角色一致 |
| t913-v1 | 角色一致 | 角色一致 |
| 旧 split-v2 | 3 个混包角色，首处 0x380，8,764 / 9,084 B | 7 个混包角色，首处 0x380，9,576 / 9,896 B |
| T921 恢复包 | 全表一致；截短、改 1 B 负控通过 | 全表一致；截短、改 1 B 负控通过 |

ROOT_TIMELINE 可用外部角色清单核分版，split-v2 应有 14/27 派发。粗档 6/7 点，细档 31/57 点。已核原 T914 baseline 细档证据，增加错查询粒度和晚 promoteExit 负控。

## 封存语义

只对已绑定外部角色清单、真实同根封存且在截止内通过 typed 校验的 SOURCE 进行比较。先校验各文件 SHA、实际 INPUT 镜像、Expected、Completion、可观测时钟顺序、命令 capsule 和来源 FNV。原生 R1 另核 composite 证书和 GPU seed。输入白名单为两族相同的 64 位对起始字 4、6、8、10、12、14、16、18、20、43。状态白名单为 4、6、8、14、16，命令 capsule 仅应用同一输入字典。核实后再重算来源 digest。EVENT 仅映射已核 nonce 和 ROOT_BEGIN 中相符的样本号。实际 seq、sourceId、载荷、配置、物理状态、EVENT 剩余字及 R1 maps/markers 比较原字。未分类差异保首字节及两侧 SHA。

原生同 seq 重复使用原 capsule 身份和完整命令比较，覆盖合规重复与载荷冲突。跨进程等价范围为首笔 SOURCE、空历史来源初态。历史来源跨进程归一、QUANTUM 以及迟退役根不得使用本判据标成等价。封存旧 T914 缺两侧完整输入和输出，因此两族的跨进程正例、业务差异、错证与重复目前使用明确的合成夹具验证。

```powershell
python test/t922/selftest.py
python test/t922/check.py package PE ASSETS t913-split-v2
python test/t922/check.py timeline RUN.jsonl ASSETS/roles.json --detail
python test/t922/check.py verify ENVELOPE ASSETS/roles.json
python test/t922/check.py diff ENVELOPE_A ENVELOPE_B ROLES_A.json ROLES_B.json
```
