# T482 coherent 缓冲值传参修复

2026-11-11，developer，不使用真机。按用户本轮指令直接实施。

T472/T477 的原始 SOURCE 编译日志为 `0:1138: S0001: Function call discards 'coherent' access qualifier.`。收据的 shaderSHA 为 `eb479a7b4fb3be0dc06245aef95b05e4ceabcaa9f9de7051994c5ac4504a1c1c`，与 T475-handoff-final 的实际 SOURCE 字节一致。该字节第 1138 行是 `m2KeyLess(W[at+m2OldRank(mid,d0,d1)],slot,k)`。

修复在调用处把 coherent SSBO 元素读入普通局部 uint，再按值传给函数。同类 SOURCE 行读取、诊断差异、状态写入也显式传递已加载值。M2 从 M1 复用的 publication 浮点位编码调用经 `m2CandidateValue` 返回 Candidate 读值，始终读取 C，保持 replay 与已提交状态的区分。

Candidate、Workspace、Output 三个 coherent 声明保留。无 volatile/restrict 新增或删除。工作组共享排序和索引更新的 memoryBarrierBuffer/barrier 保留，dispatch 之间的 GL barrier 保留。函数参数均为普通输入值，不传存储引用，不修改缓冲别名、排序键、质量数学、事务协议或同步阶段。

T452 → T468 → T475 继续从实际 GLSL 生成编译源和诊断身份。ProgramFactory::Key 使用完整源码、layout、环境及 link 配置；新 SOURCE 字节使 SOURCE key 改变，旧 artifact 无法命中新 SOURCE。两个 quantum program 字节沿用，分别验证创建与冷暖加载。失败不持久化的路径继续由原 factory 执行。

非机验证由 `test/t482/check_source.py` 检查旧故障位置和实际新源码边界，复用 T452 的 GPU root 断言验证 192/64、192/128、512/64、512/128 四配置的 SOURCE/replay/QUANTUM/负控，复用 T475 的真实生成 App 创建三 program 并验证冷编译及暖缓存。编译、签包、HAP 内 ELF 与 shader 字节核对结果记录在 `test/reports/t482/delivery.json`。

T477/T461 使用新签包继续真机 SOURCE 创建、整根及缓存主链验证。Maleoon 编译结果需该设备实际重跑。T473/T410/T437 使用 M1 来源，其 READY 后 GAP 与此次 M2 SOURCE 首错分别记录，来源与恢复证据由 T483 核对。
