# T394 · 单沙解锁与三区取证隔离

隔离包由 T385 生成器派生，启动进入 P051 单沙页。签包路径及 SHA256 见[签包回执](../t257/T394-final.json)。页面使用原 XComponent 与单沙 shader。开发验证在宿主执行，设备证据由独立 tester 取得。

静态颜色臂点「厚床三区」。隔离 owner 通过既有初始化写入 128 列各 48 单位的重沙床，再从原干沙准入入口投放 0.1 单位的孤立 AIR。正常 V2 封签、pack 和 native CURRENT 成功后，预填从只读快照挑选实际 viewport 支持的 BED、AIR、BG。每次「中性」或「纹理」重新等待自己的有效 token。依次点「预填 BED/AIR/BG」「预约一次」「导出读回」。冷档记录权威库存、128 列显示床、粒子打包字、配置摘要、两读点及原 Swap/Unbind 收据。空床、薄床、无孤立 AIR、无 BG、几何不合与陈旧输入分开记录。

动态臂分别点「对照 A」和「锁定 A」。两臂通过正常 owner 投放旧粒子，读取原支持区、新支持区和背景。各臂先导出 A，保持期间取无损全图 PNG，记录实际组件像素 bounds、截图采集时间，再点「封存截图时段」。点「注入 B」以同一窗口和光学、同一组固定 ROI 在另一位置正常准入新粒子，并由解锁后的有效 tick 产生 B。B 需要真实准入、打包内容变化和新 ID，之后同样导出、截图、封存。锁定臂最后点「重锁」，可用「导出阶段」记录确认。截图保持期间暂停后续帧提交，两臂都采用这项取证操作。几何、owner 或生命周期变更会撤销保持。常态绘制仍走原四参数调用和零诊断读回。

每次预约最多两目标共 98,304 字节。保留原 ROI 解码与 GL 状态恢复，冷档增加同一阶段的 copy、pack、draw 时长与 native candidate、back、Swap、Unbind 稳态时刻。选定粒子在权威快照中的 x/y/state/mass、准入收据和实际成功子步的 dt/角度/正 LAND 随档记录。取不到当前锁定样本时可用「导出阶段」取失败分层。保留原 native draw 收据及 prepare 返回码。

系统显示独立判读。对 A、B 各写一个 JSON，字段为 `manifest`、`hold`、`png`、`bounds`（left/top/right/bottom）、`captureWallMs`（截图工具记录的同域采集时刻）。路径相对该 JSON。运行 `python test/t394/pair_display.py A.json B.json`。工具校验 manifest sidecar、读点哈希、同 token/阶段与保持时段、无损 RGB、精确像素映射和固定 ROI，并从打包字计算选定 ID 的 CPU 可见掩膜。仅在对应 ROI 的屏幕 RGB 与 back 原字节相符时给出配对结论。映射、时钟或颜色差异使证据不足时输出 `DISPLAY_NOT_JUDGABLE`。

[生成代码离线结果](units.jsonl)覆盖两个静态光学臂、权威床总量 6144、两条动态 A/B、有效正 LAND 0.1、旧 CURRENT 遇准备失败和跨代后撤销、单次预约与常态四参数。T385 解码 17/17 与旧冒烟通过。宿主 GLES 验证双读点、98,304 字节上限、pack/read GL 状态恢复、故障隔离与失败 Swap 保留 LastGood。截图工具覆盖正常配对、重复显示以及时刻、映射和 SHA 反例。Hvigor ArkTS/native 构建与签名校验通过。

复跑：`node test/t394/generate.cjs <fresh-local-dir>`，`node test/t394/units.cjs <fresh-local-dir>`，`test\t372\build_host.cmd <fresh-local-dir>`，`python test/t394/pair_units.py`。
