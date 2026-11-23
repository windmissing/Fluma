# T855 · PC M1 SOURCE 描述符与代际回归夹具

2026-11-23 · tester · Windows PC 离线。判据来自 [T851](../../../docs/architecture-T851-v1.md)。

- `python -m py_compile test/t855/check.py test/t855/selftest.py` 通过。
- `python test/t855/selftest.py --emit <synthetic.json>` 生成 11 组样本。 `python test/t855/check.py check <synthetic.json> --synthetic` 独立读取通过。
- 固定 SOURCE 和交互 SOURCE 均覆盖 2×8 成功、少配 8、16 项含空句柄、分配失败，以及写完后 READY 前撤销。交互另覆盖冷角已提交、未完成时撤销并等待 fence。13 项反例覆盖提前更新、错误/缺失退役、过时代或线程、角表 fence 失败、漏写、把 void 更新伪报为 VkResult、句柄统计不一致与截尾。
- `python test/t855/check.py historical` 校验 T847 三份旧日志及 T851 调试日志、异常栈和反汇编原始 SHA，确认历史 8/16 根因与四份日志停在 READY 前。T840 固定 dry SOURCE 同根诊断由既有 reader 复查通过，T836 的 8 项及 T848 的 10 项负控回归通过。

## 新 PE 交接

`check.py check <normalized.json>` 接收采证方标准化的事件列表。样本字段参见 `selftest.py`。保留原始 JSONL、进程起点、owner/device/surface/geometry、池标识、PE 与三 SPIR-V SHA 和采证来源。准备/分配、update 调用入口和真实返回、Ready 的 QPC 顺序来自原件或独立探针；缺少逐槽句柄或 bank/pass 写入依据时保缺证。句柄可用同录制内稳定的整数别名，零保留为空。固定路径用 `FIXED_INIT_DONE`，交互路径用 `READY`。可提交多个独立初始化记录，每项终结于相应成功或已退役故障。用独立记录核新代初始化，旧代撤权记录不再发 READY。

交互时先核角表 245776 B 与实际 fence 返回，再核 SOURCE 16 组更新。少配、空项和分配失败不得触达更新；撤销后不得发布旧代 READY，已提交 fence 需退役。效应验证由 effector 实跑新版，再按 T836 验旧代输入拒绝，使用 T848 的 `check.py <M1PCOLD1> <同进程JSONL>` 校验实际冷档与已接受的同根 Present。历史固定路径和合成夹具只支持各自证据范围，T853 新 PE 的运行结果仍待采证。
