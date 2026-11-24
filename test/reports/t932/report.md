# T932 · PC 窗口跨代收口与 G16 新版接触实测（effector）

2026-11-24。两项独立子项。开发集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924），GPU 驻留、shader 渲染，无 Canvas 回退，无真机。证据根 `test/reports/t932/runs/`。

## ① 目标 2 收口：窗口跨代生命周期（goal2-lifecycle）

t926 PE + 紧凑档（t918-shaders），驱动 `test/t932/drive_goal2.py`：4s 持按（gen2 出沙）→ resize 800×500 → 再持按 → SW_MINIMIZE 3s → SW_RESTORE → 再持按：

| 阶段 | 证据 |
|---|---|
| 非空连续呈现 | 每代 M1_QUANTUM_PRESENT_QUEUED 连续：gen2 47 量子、gen3 40、gen5 49；各代 `windowFrame.physicsCommit` 从 2 重新计数（新代首帧） |
| resize 跨代 | gen2（624×431）→ gen3（784×461），`WINDOW_AVAILABLE` 两代几何入档 |
| 最小化 | gen4 iconic：无 `WINDOW_AVAILABLE`（wake 门 `width>0` 正确挡住 owner 重入）、iconic 期间零呈现 |
| 恢复 | SW_RESTORE → gen5（784×461），49 量子继续出沙 |
| 旧代不呈现 | 各代 present 时间区间互不重叠；gen2 在途根撤销 3 条 `windowCurrent=false reason=WINDOW_REVOKED` |
| 旧代资源退役 | `AUTHORITY_CHANGED` 25 次预约终端（旧 epoch offer 拒绝）；`M1_INPUT_WAIT_NEW_DOWN` 17 次（跨代后旧会话脉冲要求新 Down） |
| WSI vs 有效可见 | `M1_QUANTUM_PRESENT_QUEUED` 为 WSI 队列侧证据；`FENCED_READBACK` 3 行（gen2/3/5 各一）带真实 ROI 像素（T927 像素判据同链路） |
| 持按保持 | 恢复链 SUSPEND 39/RETIRED 22/NEW_BASELINE 22/HOLD_REVALIDATED 22——换代不丢持按 |

**结论：目标 2 可关闭**。resize/最小化/恢复各推进一代；新代首帧实测；旧代在途根 WINDOW_REVOKED、呈现停止；WSI 队列证据与 fenced 像素双轨留档。

## ② G16-A：新版旋转近根实测（g16a）

工件：`t909-g16-dm2-release/t909_angle_probe.exe`（b14678b7）+ `g16_isolated_contact_v2.spv`（7e1dae62，23020B，与 developer 文档 SHA 一致）。T912 冻结 52B 旋转输入（`t909-g16-fixtures/rotate.bin`）。

- **GPU 原字**：单 fence 成功（fenceVkResult=0），250ms 门内（elapsedNs=587,600≈0.59ms），`PRIVATE_ANGLE_PROBE` 收据入档。
- **T930 独立判据**（`check.py contact --near`）：**errors=[] 全过**——
  - TOI `0.17887421765` s（60 位十进制参考 0.1788742176550）；
  - **0.05ms 门**：观测 TOI 误差 **-0.000142ms**（350 倍余量内）；
  - **近根符号**：夹区间两端 gap **+2.69e-8 / -2.69e-8 m**（一正一负，符号确定夹根）；
  - 输出 gap `+0.198 / -0.190 µm`（原 v1 偏差 +0.0805ms 已修复，gap 误差界同 T930 参考量级）。
- **对比旧版**：旧 `g16`（T912）偏 0.080503ms；v2 落在 -0.000142ms——修复实证。
- 离线模型数值（developer 文档的 0.1788740754）与 GPU 原字观测 TOI 一致（同 0.17887408 内），判据以 T930 独立计算为准，未把离线值当 GPU 结论。

## ③ G16-B：13/52 盘多盘实测（g16b）——受阻（原子拒绝证据在案）

四组实测（13/52 × static/hold/fast，`G16-B13/B52.disk64` 实名种子）：**全部 exit=3，`G16_PRIVATE_ROOT` firstReason=2 firstStage=1 disk=1，`acceptedRoot:null`**——GPU 首根（freeze 阶段）原子拒绝，复现 4/4 组。

- **根因定位**：`g16_multi_v1.comp` freeze() 断言 `state[a+13]==0`（disk64 第 14 字 flags），而 T928 合同种子（`test/t928/fixtures.py` `DISK.pack(i,1,i,materialId,…,1,0,0)`，`architecture-T928-abi-v1.json` records.disk64 第 14 字=flags）**写 flags=1**——shader 断言与种子 ABI 错配，首盘（disk 1）即拒。
- **佐证**：导出 `base.disk64` 逐字等于种子原文（身份检查过、valid=true、completion 落盘）；`candidate.disk64` 全零（fail 在复制循环前 return）；本地 FP32 验算 r/m/inertia/身份字全部符合期望（唯一不满足项=word13）。
- **绕道排除**：宿主入口绑定 seedSHA（d74e…/eca4…）——改种子（flags=0）会在 `G16_SEED_SHA` 被拒，无测试侧通道。
- **未完成项**：两档两速正例、守恒/冲量残差、同根窗口、present 同根画面。
- **前置**：developer 修 shader freeze() 的 word13 断言（接受 flags=1）或重打包种子（flags=0）并更新 SHA 门。
- **接手角色**：developer（shader/种子一致性）；修复后 effector 直接按本卡命令复采。

## 结论

| 子项 | 结果 |
|---|---|
| 目标 2 跨代收口 | **完成，可关闭**：resize/最小化/恢复/新代首帧/旧代撤销全证据（WSI+fenced 像素双轨） |
| G16-A 旋转近根 | **完成**：T930 判据全过，0.05ms 门内 350 倍余量，夹根符号确定，旧版 0.0805ms 偏差已修复 |
| G16-B 多盘 | **受阻**：四组原子拒绝 reason=2（shader word13 断言 vs T928 种子 flags=1 错配；seedSHA 门挡绕道）；守恒/冲量/窗口待 developer 修复后复采 |

GPU 驻留、shader 渲染全程；私有根真实 250ms 门原值。
