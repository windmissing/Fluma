# T654 v4 · G13 包打通——页可达/so 加载/配置排队/诊断原件首采，编译墙时待解

日期：2026-11-20，effector（tester 兼，含出包），真机（011124264R000337）。
被测：T654-g13c-signed（T716 树+g13flow 集成，inputSha 8ee6e75c→signed
d92ee0fb，sign=PASS；libgrs_g13.so 1362336B 首次入包）。
证据：`test/reports/t491/t654c2/`（lC–lH 布局、g13-diag-0/1.json 诊断原件、
v4-g13-package.json）。

## 设施打通链（tester 出包域）

1. G13 页可达性：T716 树入口=G9RxResearchAbility（module.json5 srcEntry，
   非 EntryAbility）——onCreate 读 `--ps g13 true` flag 路由 G13 页
2. libgrs_g13.so 构建：主树 g13flow/ + types/libgrs_g13 + tools/g13 +
   T607 四 docs + 5 工程输入同步入 t716 树；**host 预生成 assets**
   （contract_generated.h 218271B/pipeline_source.h 263573B/layout 177883B
   等 6 件）+ kernels.cmake 切预生成拷贝模式
3. BUILD SUCCESSFUL 26s，**libgrs_g13.so 首次入包**

## 真机首采（里程碑）

- **G13 页首次直达**：「G13 · 纯流场独立研究」（配置 REST/激活/停止/冷档/
  速度/示踪/涡量/动压/总压 按钮全在）
- 配置 REST → **G13InitDiagnosticV1 诊断原件首采**：lifecycle
  **PROGRAM_BUILD**，lastProgress **SHADER_COMPILE:ENTER**，
  queuedTicket=172，typedarray 域（rawReturnedLength=512，
  providerProfileId=T629:…，providerEvidenceSHA=80ac88e1…）
- **阈值域 UNBLOCKED**：T672 同页设施+页内 32768B 四页快照路径已可达

## 剩余（精确）

**G13 shader 编译墙时**：260KB pipeline_source 编译 >90s，等待期应用退出
（无 jscrash——编译阻塞期 UI watchdog/主动退出）；重进回 UNCONFIGURED
（attempt 不持久化）。配置化快照需编译期容忍（worker/分段编译）或更短
编译路径——回 architect/developer。

## 判定

T654 采集链全通（页→so→配置→诊断原件）；阈值表在首个存活配置化快照
后即可出（T649 八阈值+T672 解析器全就绪）。
