# T743 v1 · 目标 20 G13 Vulkan BOOT 同机精度实测——**FTZ 硬件属性确认**

日期：2026-11-20，effector（tester 兼，含出包），真机（011124264R000337）。
被测：T743-vk（T742 Vulkan BOOT probe 2294c5c8，inputSha 96ac13bf 回执一致
→signed bdc2a7bc，sign=PASS；libgrs_g13vk_boot.so 394248B，映射 BuildId
e7b5586e hilog 验证）。证据：`test/reports/t491/t743/`（l1–l3 布局、
vk-diag-1.json 原件 2805B+、v1-vulkan-qualification.json）。

## 执行

EntryAbility →「G13 Vulkan BOOT · FP32 资格」页直达 → 运行资格探针（精
确点击 516,818）→ 40s。

## 结果：FAILED @ PHYSICAL_DEVICE_QUALIFICATION（NO_ELIGIBLE_PHYSICAL_DEVICE）

- Loader/Instance 正常：/system/lib64/libvulkan.so（b56a00ca），API
  4206901/4202496，扩展 VK_KHR_surface+VK_OHOS_surface，surface 创建成功
- 设备枚举成功：**Maleoon 935F**（vendorId 6629，deviceId 537137152）1 台
- 资格预检未过 → pipeline 创建/fence/op65-66 读回未到达（合同"若不具备
  资格保原拒绝"=**GLES 拒绝维持，不换产品后端**）

## T741 三 FP32 属性（同机 Vulkan 实测——核心答案）

| 属性 | 值 | 意义 |
|---|---|---|
| **shaderDenormPreserveFloat32** | **0** | **次正规不保留（FTZ）** |
| shaderRoundingModeRTEFloat32 | 1 | RTE 舍入支持 |
| shaderSignedZeroInfNanPreserveFloat32 | 1 | 0/Inf/NaN 保留支持 |
| denormIndependence / roundingIndependence | 1 / 1 | 独立性支持 |

**关键结论**：同机 Vulkan 侧实测确认**次正规 FTZ 是硬件/编译器属性**
（denormPreserve=0），与 GLES op65 +53/op66 探针清零**跨 API 一致**——
不是 GLES 驱动特有 bug。G13 BOOT 门 MUST-PASS 在本设备上不经设计决策
（FTZ 安全向量或门放宽）无法满足。

## 原件

vk-diag-1.json：pid 49047/processStartTick 2380458/ownerTid 49291/
mappedElf/moduleSHA cb82cd06/sourceSHA 8b688c5b（parentSourceSHA
a93aecf8=BOOT 源一致）；GLSL 24160B 与派生 SPIR-V 116688B 已按合同登记。

## 回 architect

三选一决策输入已齐（A) FTZ 安全门向量 B) precise 模式 C) 门放宽）；
跨 API 证据补全——次正规 FTZ 为设备级属性，非 GLES 路线可修。
