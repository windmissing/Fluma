# 2026-11-19 T619 接续完成

T619 正式合同已消费，原 PREFIX_IDENTITY_CONTRACT_REQUIRED 发布阻断已移除。入口锁内冻结 RootPermit/ownerBefore 与原 scope18；最终授权后首个原业务 dispatch 成功提交才置 ENTERED，未进入不冒已执行；不拿结束 owner 反算入口。

公开 rootCursor64、完整 4096B 配置胶囊及同根 INPUT64/quota/completion/record0..29 的 2920B LE32 流式 token 已接实际 writer。配置在 Transport 的单次 Create/Release 生命周期内不可变；Release 清七份旧根，禁止拿新配置补旧 accepted。自有 program 冷初始化明确将 uKappa 设0，不启用新水模型，不增加热上传/dispatch。

Native 严验及有界 Python reader共同核 capsule/cursor magic/revision/seal/reserved、scope/root/input/参数/SHA 与 token；缺合同/未知版本具名拒绝，NOT_EXECUTED 原件槽全零，失败根保持 FORENSIC。仍保 Meta2/paired streams/BASE/SOURCE_CONTEXT、原预约与真实退休链。固定 metadata 七实例实际 sizeof 增量编译断言2296B，总量由既有 getter实际计账，非payload/诊断信用。未新增 GPU bank、dispatch、hot map；原40 copy/5120B保持。

本卡开发、native/ArkTS/unsigned HAP编译打包完成，原 Python reader仅 py_compile 未执行。未跑测试或真机，未签名部署；不冒 FULL runtime/守恒/设备 PASS。最终原件与源码指纹见 `test/reports/t596/development-t619.json`；旧报告为当时历史，不覆盖其构建身份。

---

# T596 / T614 接续回执

2026-11-18，developer。不测试、不用真机。T614入口合同已实施；整张T596仍未完成，状态看评论，FULL未开放，T594未据此解锁。

已交：SCOPE_INIT9唯一lane清cursor32；INPUT_E_REQUEST15中首credit前封不可变entry9/10及nonce/role/branch/revision2/seal；内部replay只重置mutable0..8并核原入口身份。host切点改为15后copy，尾在SCOPE_FINALIZE后copy，四Q仍40次/5120B/0新增物理dispatch。正式JSON生成capture_schema_generated.h/.glsl，实际quantum生成与CMake源/hash同步，旧核不能冒新wire。

Meta2从同role原entry/final/control构造，checked差值与共同sealed、nonce/branch/role及root相互核；控制公开体先保6×128行，再构5×Meta32并清928..2047，内部raw不外泄。FULL CONTROL容器rev2，ROOT CONTROL仍1，非prefix其它段仍1。writer/validator及新只读reader核对应版本与paired counts/ranges。

已接完整native cold预约→等待下一plan边界→owner准备GPU保留→原根推进→caller-owned span采集→发布/失败的生命周期框架。全进程单FULL GPU票据在分配前保留，GL owner安全delete才退款；丢失context时保retiring信用至真实eglDestroyContext成功回执。native双slot仍共用此前broker，并核本profile/mode pair上限。没有Native第二整档vector。GPU保留5(S+D)、双stream、控制与输入额度沿原公式；新固定CaptureRoot元数据计入capability的fixedCaptureMetadataBytesExcludedFromPayload，明确属于旧包络排除的fixed objects，不冒原包络已含该固定对象成本。

逐根保存真实permit、冻结INPUT、完成尾、quota、source控制generation及原SOURCE命令；BASE在首次写之前copy。跳过判定取真实PlanGrant/sourcePhase与实际控制NOT_EXECUTED原件，缺对象不以host mask冒跳过。frame从accepted state的真实arena/rows获取；AUTHORITATIVE附属copy加实际auxStart，避免拿前根的aux冒本scope。SOURCE上下文冷初始化未用槽规范0。新增结构均为原件/身份搬运，不改变物理选择/配方。

有界GpuV5Cold逐页保存原JSONL，四个流式SHA分别覆盖canonical prefix/raw prefix/payload/pages file，最后manifest单独落盘后改名；新增reader以临时文件流式读，不执行流体/粒子物理。collector/reader仅交代码，未执行。ROOT/FULL发布仍由实际native writer验证；当前缺定义时显式PREFIX_IDENTITY_CONTRACT_REQUIRED，不输出伪COMPLETE。full配置入口同步具名此原因，默认off不变。

实际阻断（原卡评论6afd8837e4b0dc337e97c48d）：T527只给rootCursor64地址，缺逐word/usedMask/原producer切点；RootRecord[58..65] rootTokenSha缺原字节与哈希前像；4096B fullColdConfigCapsule缺数值字段表。T614明确未知字段不得借私有cursor填充。已查T527/T535/T552/T614机器表及当前源码，不能反算/自创编码/填零冒动态身份。需architect补正式字典/前像，或明确全零reserved和可逐字搬的既有原件。然后developer仍须完成该prefix编码、root token绑定、reader相应完整校验并解除整体发布门；不是只缺测试。当前仅有这些字段的缺失占位，writer明确失败，reader也不报完整。

编译：新生成.codex/local-test/t596-t614-delivery的native/ArkTS/unsigned HAP成功，完整日志t596-t614-delivery-build.txt（70.425s），最终冷IO同步后t596-t614-sealed-build.txt再次成功（17.063s）。此前T614切点版和collector版也完整编译通过。未运行shader/reader/单元/模拟/设备测试，不签名部署。准确源码和HAP/ELF身份见test/reports/t596/development-t614.json。所有上屏仍XComponent+shader；CPU仿真停投。
