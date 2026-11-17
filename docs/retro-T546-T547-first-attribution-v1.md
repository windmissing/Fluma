# T546 / T547 architect交付回执

2026-11-17。不使用真机、不运行产品/NAPI/GL测试，不改业务实现或tester原件/报告。仅本人任务状态有限例外；本轮两卡约定的首次归因、工程修订、机器表和精确缺证分流已完成。

T546：原jscrash确定G9 start:43 JSON.parse未捕获崩退；native helper隐藏首个拒绝谓词、启动未核configure成功便发frame，工程缺口已冻结修复。canonical helper与T522-delivery/T529-delivery/T529-development生成件归一化逐字一致；设备上具体NAPI status/长度/指针/语义判定仍无原件，不采“typedarray同根已定”的报告断言。主稿 `architecture-T546-napi-rejection-and-safe-reply-v1.md`，机器表 `architecture-T546-input-rejection-manifest-v1.json`：严格N/uint32/offset/bytes/detached门不放宽，新增有界逐谓词诊断；JSON端点统一具名错误对象，旧状态接口兼容；G9/M3/M5启动/解析/清理完整catch。developer先实施此确定工程范围并编译提交，正确新包交tester作最小一次入口谓词补证，再回architect定真正配置拒因，不越过缺证猜修。

T547-Create：原始C0 378行189对；C2 660行包含逐字C0前缀，C2自身282行141对均为glCreateProgram。C2报告的18次compile/环境miss推断不成立；CREATE_WAIT仅主线程轮询。当前sizeof门仍双0B、runner路径/按image杀进程亦有缺口。新增 `architecture-T547-create-and-empty-export-triage-v1.md` / `architecture-T547-create-export-manifest-v1.json` 精确冻结EGL/binary/查询/reflection/resource/prepare/scene及commit/recovery调用表、每call/attempt身份、IN_API与sink I/O区分、匹配源/ABI/环境DLL的完整seed、独立arm目录和精确child-PID 900s守护。独立复算当前SCENE_FULL key为e3c4f777…；现存目录收据环境SHA同2aabb8c8…，仅上下文信息，不拿历史混合收据冒本次六role暖成功。仍缺的具体卡点交tester按该清单补证；若高层观察seam缺失则精确转developer，不默认shader拆分。

T547-导出：canonical `test/t468/M2CreateColdFiles.ets` 与T482-final生成件逐字一致；空hex合法，0B原件与现场update failed提示支持收据链在空md.update失败。修订T521旧缺证判断后交developer：仅空文件成功fsync/close之后返回标准空SHA，不调empty update；非空及I/O/hash失败仍严格失败。原native/manifest/WS_EVENT协议不变。旧mock接受空update，后续独立tester须模拟平台空update抛错，不以旧mock PASS消除真机缺陷。修复后T461才能补完整诊断链，再判create/READY/root，不猜物理失败。

只读审计/有限契约程序：`python docs/analysis-T546-T547-engineering-audit-v1.py`；T546 49项、T547 39项通过，证据分别 `analysis-T546-rejection-evidence-v1.json`、`analysis-T547-create-export-evidence-v1.json`。检查包括正式原件SHA/逐call配对、文件前缀继承、实际布局与空自动门区分、完整key/封装复算、严格首拒因/安全JSON/最坏4096B错误响应以及空raw数学和I/O前置向量。PASS_FINITE_ENGINEERING_ONLY，不等NAPI/产品/device/物理/效果通过。

两个旧工程文档仅加显式新证修订引用，未篡改tester历史结论。manager可直接登记两卡交付并派上述角色接力，不新增manager验收门。developer职责开发/编译/提交、不执行测试；tester/effector独立验证。CPU停投、新XComponent+shader唯一上屏、失败FAULT/标明STALE、250ms/GAP/120Hz配额/30fps显示与defaultOFF均保持。
