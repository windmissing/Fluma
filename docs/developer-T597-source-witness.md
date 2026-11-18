# T597 SOURCE窄见证修复

2026-11-18。仅开发编译；不测试/不用真机。

真实test/t537生成桥的队头hook已去掉App mutex二次加锁。M1及实际M2route Run同步记录SOURCE自身的plan/nonce/intent/sourceCommit，提交与终态来自真实SubmitSource/Poll/AcceptedRoot，不读旧Q latest nonce。Offer返回OK才CLAIM_ACCEPTED，ROOT_STARTED之后才填写SOURCE身份；terminalCode为真实outcome，失败以高位区分TransportCode。单观察serial旁路只写诊断，不更改队列/nonce/控制唤醒。

Ledger恢复单指定enqueueSeq；Witness及整个Ledger均static_assert<=256B。按完整scope/intent/seq匹配，旧scope不被晚回覆盖，terminal一经封存不被重复intent重开。ENQUEUED/ELIGIBLE/CLAIM_ATTEMPT/CLAIM_ACCEPTED/ROOT_STARTED/TERMINAL分别标1..6；NoClaim保首次实际门。JSON追加信用<=2048B，溢出使用原完整INCOMPLETE fallback，不抬2MiB/4MiB限额。copy nothrow空值处理和HostScope/原Gate RAII均保。

生成器显式将canonical M1改动复制进冻结旧链，对M2route生成源做同一观察补丁，防止只修未被编译的源码。M1、M2最终hvigor/HAP编译通过，分别t597-credit-build.txt（13.021s）和t597-m2-final-build.txt（13.249s）。没有执行单测、GPU、真实按住或设备验证，T593需独立采同scope真实Down跨pulse链；编译不证明该链实际已发生。
