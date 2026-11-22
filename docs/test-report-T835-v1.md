# T835 v1 · PC 实际窗口诊断帧独立验证

日期：2026-11-22，effector（tester 兼），Windows PC（无手机）。
被测：T830 handoff PE（43fed568 回执核验一致）实跑真窗口。证据：
`test/reports/t835/`（run1.jsonl 17 条全阶段、v1 判定）。

## 实际窗口链路（17 阶段独立复现）

WINDOW_AVAILABLE（**hwnd 86117998 真实窗口**，784×461/dpi96/60Hz）→
VULKAN_LOADER → INSTANCE_CREATE → **WIN32_SURFACE** →
PHYSICAL_DEVICE_QUALIFICATION → DEVICE_CREATE → PIPELINE_LAYOUT →
BOOT_SHADER/PIPELINE/BUFFERS → RECORD → QUEUE_SUBMIT → BOOT_FENCE →
FENCED_READBACK → **BOOT_DIAGNOSTIC_WSI**。

**硬件 adapter 确认**：Intel UHD 630 ×2 枚举（选 0），queueFamily 0
flags=15（graphics+compute+present 同队列），surfaceSupport=true，
floatControls=Vulkan 1.2 core（denormPreserve/RTE/signedZeroInfNan
Preserve 全 1），扩展 VK_KHR_surface+win32_surface+swapchain。

## 诊断帧（核心验证）

| 项 | 实测 |
|---|---|
| kind | BOOT_DIAGNOSTIC |
| **中心像素** | **0xff00ff00（=4278255360）与 T830 回执精确一致** |
| acquire/submit/fence/present/imagePresent | 全 **0**（零错误链） |
| stale/error | false/0 |
| swapchain | 2 image/format 44 |
| present 窗 | 135μs（QPC 15174067698520→...655） |

## GPU 计算域

op65/op66 **QUALIFIED**（tailSeal 3235779329/tailError 0），队列提交/
fence wait 全 0，stagingCoherent=true，dispatch 8 笔（op64×5+op65+
op66×2），ctrlTail 73728/work65 583296——原 BOOT 精度域在 PC Vulkan
复现 **QUALIFIED**（与手机硬件 FTZ 域不同的结果，PC floatControls
路径 denormPreserve=1）。

## acceptedRoot=null 诊断范围（按卡面明确）

本卡验证**窗口/surface/swapchain/像素/present 管线**——非空业务首帧
（SOURCE）与持续性能**不在本卡范围**，待实际产物另取。

## 回执

PC 实际窗口诊断帧**独立验证通过**：真 HWND/Win32 surface/硬件
adapter/swapchain/精确像素/零错误 present 链全复现。
