# T384 · GPU 提交停顿分段探针

签包及 SHA256 见 [签包回执](../t257/T384-final.json)。沿现有 grsm1 启动路径。在“光学/诊断”点“下轮提交探针”切换，再点“新空场”；开关按新 run 锁定，默认关。同设备先关复现、再开定位；各轮用原 T306 冷导出和签包 SHA 登记。开发未使用真机。

M1 的 T384_STAGE50_V1 保留旧 1–42（包括 23 历史桥层标记）；M0 继续旧字典。43 BatchCall 与 44 Flush 按 epoch/run/surface/TID/root/carry（批号）配对，A/B 是半开 pass cursor，44 code 0/1 区分非末/末。probeStart/End 为调用边界原始 steady ns，ns/now 为入环时间；两次 flush 采样之间没有写环操作。probeWriteNs 单列回调至提交环写前的开销，包含等待锁；不含最后一行复制和计时读本身。43 包括已有 op/fence 观测，因此只把 44 区间解释为 GL 调用墙时，不推断 GPU、驱动或脱调度归因。

45 是 ContinueSubmit 的返回；46 是 Run 进入 inflight，carry 是 root kind；47 是实际 FullM1Transport::Poll，code 保留真实返回，A/B 为累计 accepted 前后，C 为 root kind；48 是实际 accepted，code 为 publication slot，carry 为 kind，A/B/C 为 generation/source/physics；49 在撤销前记 GAP，A 是 anchor，B 是当时 accepted。50 对应原 Draw/Swap 13→15→16→14，carry 是 slot，A/B/C 同 48，code 的 1/2/4 位是 current/swap/仍属本 run，附完整 requested/rendered token。32 个固定 token 槽用尽时明确输出 MISSING_BOUNDED_RECEIPT。冷分析应对缺行、跨身份和 GAP 撤销样本拒判。

离线入口：node test/t384/units.cjs；node test/t384/read.cjs <NDJSON 分片...>；cmd /c test\t375\build_host.cmd。旧 T376 三片可读，3 个 stage23 只认作桥标记。GLES 512×128 已通过 50 个断言，覆盖 14 对批次、真实 BUSY/OK 和实际接受身份。冷导出、交换和 GAP 的设备证据由 T376 取得。
