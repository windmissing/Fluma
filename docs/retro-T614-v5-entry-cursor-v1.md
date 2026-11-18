# T614 入口游标工程复盘

2026-11-18。T552此前把GPU函数内部切点描述为host dispatch后copy可取，忽略BeginScope和首aux事务在同一invocation；还混淆SCOPE_INIT与实际INPUT_E_REQUEST枚举。T596的具体阻断成立。修订见 [入口合同](architecture-T614-v5-entry-cursor-v1.md)。

末尾减一不能保原件，额外dispatch/热map又越既有预算。选择在原cursor32具名保不可变入口，并在实际INPUT_E_REQUEST后copy，容量/dispatch/事件物理均不变；FULL CONTROL与Meta升版，避免旧reader默许新含义。内部重放保首次入口，SCOPE_INIT先清残值，公开Meta与内部raw布局分开序列化。

原T596继续完成原有全部封存/信用/reader责任，FULL关闭和META_INCOMPLETE保真；本次静态合同检查不是实现或真机PASS，不重建同一开发卡，不添加开发测试/manager验收门。
