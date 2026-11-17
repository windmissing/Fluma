# T545 · GPU M3 V4 四档真实设备期限补证 v1

日期：2026-11-17，tester，使用真机（011124264R000337），设备串行。
被测包：T522-delivery-signed.hap SHA256 `70d2742c6a76d11bc6aef5450a3e8e68df6c6a513ef7b9969cec54aadb49c3d7`（与卡面逐字一致）。
证据：`test/reports/t491/t545/`。

## 设备包身份链（全部逐位核验）

1. 安装 T522 签包（bm clean 后冷启 PID 16601）。
2. 设备 so：`/data/app/el1/bundle/public/com.example.helloworld/libs/arm64/libgrsm1.so`
   sha256 = `69ae39a9…` == T522 签包内 `libs/arm64-v8a/libgrsm1.so` **逐位一致**。
3. 签包 so 内 m3a kProfiles 四档表完整：条目 stride 184B，{192,64}/{192,128}/{512,64}/{512,128}
   四条目在 offset 2390424/2390608/2390792/2390976 全部找到（二进制核）。
4. 离线探针（同树头文件编译）：`SelectProfile(192,64)→FOUND`、`SelectProfile(192,128)→FOUND`。
5. 页面 ArkTS 配置词（GpuResidentM3.ets:90-91）
   `[4,15,192,64,7680,8388608,1048576,65536,1048576,0,0,120,524288,1048576,1,0]`
   与 native `m3Configure` 校验条件（app_bridge.cpp:1699：words[0]==4、words[1]==15、
   SelectProfile(words[2],words[3])、words[15]≤1、m3Words typedarray 契约）**静态逐条核对全部应通过**。

## 实机结果

M3 V4 页面（路由：Index→体验页"打开 GPU M3 V4"按钮 → GpuResidentM3）可达，按钮：
新建运行 (210,358)/完整分支新运行 (658,358)/加水 (1050,358)/湿沙 (1223,358)/
导出 V4 冷快照 (628,526)/−15°/0°/+15°（现场 dumpLayout bounds）。

**点"新建运行"（192/64 档）→ message=`M3_CONFIG_REJECTED`，稳定复现。**
四档入口被 native 配置校验拒绝 → **SOURCE72/QUANTUM549/完整身份/SSBO/shared/资源/
root 250ms/接受内容帧全部 NOT_REACHED（四档设备域零执行）**。

## 排除过程（为何定位不出谓词）

- 设备 so 逐位 == 签包 so ✓（排除旧包/串包）
- 四档表在设备 so 二进制完整 ✓（排除表缺失）
- 离线同头编译 SelectProfile(192,64) FOUND ✓（排除表逻辑）
- 页面词与 native 校验静态逐条相符 ✓（排除词序/值错）
- hilog 无 napi 侧日志（m3Configure 拒绝路径无日志输出）——精确拒绝谓词
  （m3Words typedarray 契约 vs words 值校验）无法从现有证据区分。

## 判定与移交

- **M3A10 四档真实设备期限：NOT_REACHED**（入口被拒，非超时域）。
- 最小切点 = `m3Configure`（app_bridge.cpp:1699/m3a napi.inc 同式）在真机的拒绝谓词：
  候选 ① ArkTS↔napi typedarray 契约真机差异（backing/pointer/base 相等判定）；
  ② SelectProfile 设备表实际返回 null（与二进制证据矛盾，低概率）。
- **回 developer**：给 m3Configure 拒绝路径加 hilog（打印 words[0..3]/words[15]/
  m3Words 各子判定），一轮即可定位；**回 architect** 排期。
- 不放宽 GAP/期限；软件 ANGLE CLOCK_GAP 结论不等真机（卡面）——本轮未达期限域，
  无期限数据。
- 设备已释放（不与 T542 抢）。

## 原件

l1..l6.json（各阶段 layout）、s1.jpeg、rss-phase1.txt、探针 cpp/cmd。
