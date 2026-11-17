# T541 v2 · 目标9 GPU 差分执行（T549-frozen 设施真机域）——设备资格谓词精确归因

日期：2026-11-17，tester，真机（011124264R000337 / Pura 90 Pro / MLN-AL00 7.1.0.566）。
被测设施：T549-frozen `g9_differential`（arm64 ELF 3,640,304B，git e429ea7 冻结 g9spf），
真机可执行（`/data/local/tmp/g9lab2/g9d.exe`，无参 usage 提示正常）。
证据：`test/reports/t541/`。

## T549 设施真机打通（tester 域完成）

- 传输：`hdc file send` 路径污染/嵌套问题绕过（双斜杠目标+重命名入位）
- 输入五件套按 T549 合同构造：config.u32（16word K192/参数/research/synthetic）、
  identity.u32（18word run=1 全 1）、seed.u32（591,872B 零态=stateBytes 字节口径）、
  commands.u32（1 行 SOURCE 结构）、quota.u32（256word）
- 真机执行 `g9d.exe input out`：`execution.jsonl` 产出（ACTUAL_GL_EXECUTION，
  renderer=Maleoon 935F、k=192、residentPayloadBytes=51,120,768）
  首行 CREATE result=**4 (UNSUPPORTED)**

## 根因精确归因（谓词级探针链，四步）

1. `gl_caps_probe`（sysroot gl3.h + 手写宏）：查询全 0——**宏值错误干扰项**
2. `gl_caps_probe2`（精简 EGL 属性）：仍全 0——排除 EGL config 属性假设
3. `gl_caps_probe3`（sysroot gl31.h 真值宏）：**bindings=32 blocks=16 shared=32768
   lanes=1024 fragment=16 maxSSBO=1GB——除 vertex 外全部合格**
4. `g9_predicate_probe`（直接链 frozen g9spf schema+生产谓词复算）：
   - `SelectProfile(192)=FOUND`
   - pre-query 错误干净（egl=0x3000 SUCCESS，gl=0）
   - **vertex=0（GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS 返回 0）**
   - needMax=11,718,656（arenaBytes）< maxSSBO 1GB ✓
   - **PREDICATE=FAIL——唯一失败项 vertex>=1**

## 根结论

**Maleoon 935F（GL ES 3.2）驱动不暴露 vertex-stage SSBO（=0），而 T529 生产
`Transport::Create` 谓词硬性要求 `vertex>=1` → 真机 Create 恒 UNSUPPORTED。**

- 这与产品页面路径同一谓词——不是 harness 设施缺陷、不是 seed/输入口径问题
- 也不是 T546 typedarray 域（本卡 g9d.exe 不经 napi/typedarray）
- 全部卡面差分域（S/F/P writer、动量能量、来源/互斥/容量、去重、J 资格、replay、
  typed completion、冷档、EVENT/PATH、资源界）→ **NOT_REACHED（设备资格谓词 fail@vertexSSBO）**
- K512 及 FULL 变体同理（同一谓词先拒）

## 移交

- **architect**：T529 `transport.cpp:27` 谓词 `vertex>=1` 与 Maleoon 设备域冲突——
  需裁决修订方向（SSBO 重排至 compute/fragment 端 / 谓词按 stage 能力分层 / 指定支持设备域）
- **manager**：差分重跑前置=T529 生产代码按设备能力修订+重编译（T549-frozen 重生成），
  之后 T541 沿同一设施通道直接续跑（输入五件套与执行链已打通）
- 设备原件：`g9d-execution-k192.jsonl`（ACTUAL_GL_EXECUTION result=4）、
  glver/glext/glpred 三探针输出、probe 源码与输入五件套全套在 `test/reports/t541/`

## 不冒 PASS 边界

- 未取得任何物理差分数据——全部差分域 NOT_REACHED 如实
- fixture/输入为最小合法构造，不伪造"已执行"证据
- T520 oracle 期望源就绪但无 GPU 消费对象（Create 未过）
