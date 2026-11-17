# T542 · 目标9 GPU 真机资格与根成本取证 v1

日期：2026-11-17，tester，真机（011124264R000337 / MLN-AL00 7.1.0.566，Pura 90 Pro）。
被测：T529-development-signed.hap `9290f550…`（卡面逐字核验）。
证据：`test/reports/t491/t542/`。

## 实机结果

路由：Index→体验页"打开 G9 S/F/P 独立研究"→ GpuResidentG9 页可达
（K192/K512/FULL 新运行/模型掉沙/模型脉冲/冷导出按钮在位，现场 bounds）。

**点 K192 新运行 → 应用崩溃退出（JS SyntaxError，Faultlogger 原件在档）**：

- jscrash 原件：`jscrash-com.example.helloworld-20020230-20261117112549830.log`
- Reason: `SyntaxError: Unexpected Text in JSON: Invalid Token at position 0`
- 位置：`GpuResidentG9.ets:43:25`（start() 内 `JSON.parse(native.g9ResearchConfig(...))`）
- 机理：native `g9ResearchConfig` 返回错误串 `"G9_RESEARCH_CONFIG_REJECTED"`
  （g9Words typedarray 校验或 SelectProfile 失败），页面直接 JSON.parse 错误串 → 未捕获异常 → 进程退出
- 复现 3/3（PID 26804/28760/30232 三次均崩）；Rss 417MB 时崩。

## 根因（T542/T545 同根，跨两卡独立复现）

**T522/T529 新 napi 接口（m3Words/g9Words）的严格 typedarray 校验在真机不满足**：
- 老接口（M1/M2 configure 等）用 number/double 参数——真机一直正常；
- T522 M3 与 T529 G9 首次用 `Uint32Array` + 严格校验（`base==pointer`、`bytes==N*4`、
  detached 检查）——真机全部稳定 REJECTED（T545: M3_CONFIG_REJECTED；T542: G9_RESEARCH_CONFIG_REJECTED）；
- 排除项：设备 so 逐位==签包、kProfiles 表二进制完整、离线 SelectProfile 探针 FOUND、
  页面词静态逐条符合校验条件。

精确谓词（base!=pointer vs bytes vs detached）现有日志无法区分——native 拒绝路径无日志。

## 资格域判定

- K192/K512 真实 shader 编译：NOT_REACHED（配置即拒）
- 4SSBO/shared/对象峰值、INPUT/SOURCE12/QUANTUM2024、dispatch/Flush/fence/map/Swap/250ms、
  30fps 有效新物理：全部 NOT_REACHED
- ROOT/FULL 两 slot 资源与系统 RSS 分账：NOT_REACHED（K192 崩溃在配置层）
- SYNTHETIC 模型输入不冒真实手势 ✓（未达该域）

## 移交

- **developer**：g9Words/m3Words 拒绝路径加 hilog（打印各子判定结果）一轮定位；
  修复真机 typedarray 契约（放宽 base==pointer 或改用 napi_get_arraybuffer_info 主路径）。
- **architect**：T542/T545 同根因合并排期（两卡同修）。
- 不重复无限等 Create；设备已释放。

## 原件

l1..l6.json、s1..s3.jpeg、rss-p0.txt、rss-raw1.txt、crash-hilog.txt、faultlog-hilog.txt、
jscrash-latest.log。
