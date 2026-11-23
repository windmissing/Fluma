# T863 · PC M1 冷档完整容器独立复采

2026-11-23 · effector · 真实 Windows PC（Intel UHD Graphics 630，Vulkan 1.2 core）。用 T859 PE 与原三 SPIR-V 复采，同进程交付未改写 M1PCOLD1/JSONL，交 T848 复核。

## 工件

- PE：`.codex/local-test/t859-release.exe`，SHA256 `4898a30e…2d11a`（T859 修复版：WriteM1Cold 封套 72 B、完整 32 字节摘要）。`t859-cold.exe`（`fbf1c061…c0fb`）是离线格式自测件，实跑 exit 0（"M1PCOLD1 complete write and four short writes passed"）。
- 三 SPIR-V 与 T856/T853 一致：`1912af9e…c7162` / `b8f0e327…8c85c` / `caf0614c…906ce`。

## 复采结果（`run1.jsonl`，36 行）

真实 Down→（3 s 持按）→Up 注入（PostMessageW）。链路完整：

- 角表 fence=0（245776 B）；描述符 16/16 非空、16 组 update
- SOURCE owner=0、fence=0，QUANTUM owner=0、fence=0
- `M1_QUANTUM_PRESENT_QUEUED` → `M1_QUANTUM_PRESENT_RETIRED`（publicationReleased=true，wsi=0，DeviceWaitIdle=0），同根 windowFrame（acceptedRoot == m1QuantumNonce，run/config/sourceCommit=1/physicsCommit=2/generation=2 全对齐）
- Q20=135414

## 冷档容器核验（未改写原件 `m1pcold1.bin`）

| 项 | 值 |
|---|---|
| 文件布局 | magic(8)+sizes(32)+digest(32)+raw —— 原始产物，无任何改写 |
| 文件字节 | 94408 = 72 封套 + 94336 载荷 |
| 存储摘要 | `a2ca440b…6bee`（完整 32 字节） |
| 独立重算 | sha256(raw) 与存储摘要逐字节一致 |
| JSONL 一致 | m1ColdPayloadSHA/m1ColdBytes 与文件完全一致（3 处收据） |
| sizes | (1024, 29696, 512, 31744, 29696, 512, 1024, 128) |

## T848 join 复核（`test/t848/check.py`，exit 0）

- 判据全过：COLD_RECEIPT / NONCE_OR_GPU_WORDS / OWNER_OR_FENCE / PRESENT_IDENTITY / PRESENT_RESULT / RETIRE_OR_SCOPE / ROOT_LINEAGE / EVENT_LENGTH / SOURCE_COMMAND / 容器 FULL_SHA256
- lineage=**NEXT_FRAME**（source frame 1 → quantum frame 2，permit 1→2，eventStart=0；现行 checker 已接受该谱系）
- 八段 sections 逐一带 SHA256 清单（见 summary.json）

## 结论

T859 修复后冷档容器以原始未改写形态通过 T848 全链复核；T856 期间发现的 24 字节短写缺陷不再复现。同进程 M1PCOLD1+JSONL 已交 T848（本目录）。全程 GPU 驻留+shader 管线，无 Canvas 回退。
