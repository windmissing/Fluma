# T303 隔离事务 runner 阶段观测开发交付

2026-10-30；developer；不使用真机。按 T299 工程定位缺证，未猜测或修复AVL/GPU死锁。T306已先交45dfbc1，本卡随后实施。

`test/t303/generate.cjs <fresh .codex/local-test/...>` 以原 T250 BEFORE v10 生成器为父，串行叠加阶段观察及 refine（全文件指纹/最终source身份同步到ETS/native CMake）。不改公共M1 owner或生产默认，不覆盖T250/T278历史生成器。实际交付工程 `.codex/local-test/t303-final`，生成 manifest 封存于本报告证据目录。

launcher 在 Want合法、Worker创建、post前、onerror原对象、onexit原code和终态记行。worker在收命令后立刻记RECEIVED和run/PID/TID/backend/arm/fixture/suite/faultPlan身份；每个suite配置另身份序号。native configure ENTER/EXIT、fixture ENTER、每root ENTER/EXIT、编码 ENTER/EXIT、文件写/fsync ENTER/EXIT、TERMINAL全部有固定字典。原physical错误码/异常、原回执/成功前缀保留；firstError不被后错覆盖，observerError独立。sourceManifest与实际签包unsigned SHA由原config字段及签包清单关联，不伪称运行时计算签包哈希。

固定1024×8 F64=65536B，最多16个cold identity；首存不覆盖，满时dropped/INCOMPLETE。每阶段短ROW落独立 `.launcher-stages.ndjson` / `.worker-stages.ndjson`，写全后fsync，挂在原native调用中仍有前面的ENTER。文件含HEADER/IDENTITY/ROW/ERROR/END；没有END表示未终态，不能凭空文件/末端stdout推backend死锁。日志只报有限终态摘要/文件名，不把整trace塞hilog。观察写失败被隔离，不替换物理错误或让原流程抛新错。stageEnabled=false可关闭worker lane，启用时额外fsync开销会如实落入实际服务/墙钟分母，不宣性能等价。原NativeTransport仍在同Worker/env EGL线程执行；无共享XComponent/Canvas。

非机9/0：实际生成Worker的backend/native/encode/fsync四点原错误和最后ENTER阶段；overflow；observer IO fault不改返回；ON/OFF result相同；实际legacy夹具authority/原rootRows和receipt一致（仅runId身份和physicsTimeMs运行时长显式归一，不抹库存/物理码）。12组合原回归全绿，两K×两臂×三backend，其中GPU仅显式CPU transport double；非device GPU证据。launcher错误/退出/post接线有源码检查，真实worker启动/进程退出/文件留存由T304设备验证，不增加开发门。

隔离hvigor PackageHap BUILD SUCCESSFUL，签名/验签PASS。`test/reports/t303/delivery.json` 含测试/最终manifest/包SHA；签包 `.codex/local-test/t257-sign/T303/T303-signed.hap`，SHA256 `46b6ebea96f7ade86dad170873ab84582a0280709d65ea0c46b5fb0a33c7927c`。未使用真机，不宣三backend/双臂20cut设备全通过或收益PASS。
