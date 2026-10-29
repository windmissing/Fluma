# T272 产品桥接修复

2026-10-29，developer，不使用真机；消费T269。不改run/control、120Hz、F≤4、completion256 wire或审核。

`DecodeBridgeCommand`是实际桥与Node addon共用的NAPI入口：逐项保留首stage/napi_status/argc/type/length/offset/backingBytes/detached，严格ArrayBuffer128或U32全视图32元素且backing128/offset0。页面优先传ArrayBuffer；复制32word后按generated offsets核magic/rev/reserved/features、完整run/input/geometry/config U64和raw binary64域。READY权限前不入队，ENQUEUED_NOT_ACK明确非源终态。page只接受ready快照，失效立即丢缓存，使用U64双word身份；重复setActive(true)不重建run，用户按钮独立newRun。

CLOCK_GAP严格delta>33333333：先Pause，清app/run未准入队列、input增代、carry0，随后owner零等待Pump退休quarantine，保留已接受bank历史；显式NEW_RUN_REQUIRED，由用户冷开新空run。不采用同bank自动rebase，不钳delta、不补跑墙钟。BUSY/BACKPRESSURE保留队列重试；原非法advance保具体整数码与首stage。窗口/前后台epoch撤销权限；后台间隔不混入新run锚。首正tick保持原Fixed120Clock行为。

状态输出完整双word身份、两FIFO深度/offer原码、NAPI字段、clock delta/carry/quanta/phase、cancelled/accepted、rootNonce与commit。冷diagnostics固定保64次NAPI入口32word、最多32份真实已接受completion256，满报dropped；stage失败不伪终态。导出包与签包SHA配对，不用UI状态当产品ACK。

真实Node NAPI17/0，正反例覆盖128B两路径/原位型、argc/type/31/33/offset/backing/detached/完整高word身份/非有限输入。此为Node运行时真实C ABI，不是OHOS真机ABI。真实Fixed120Clock/ResidentRun+transport double14/0覆盖dt0/首锚、8ms/carry、超窗拒绝、FIFO16背压、Pause在途取消、输入增代/非阻塞退休与此前accepted前缀不变。native+ArkTS+PackageHap BUILD SUCCESSFUL，证据`test/reports/t272/{napi.txt,clock.txt,package.log.gz}`；签包verify-app/profile见`t257/T272.json`。

开发修复已交；真实窗口稳定前台、CLOCK_GAP与New run、产品terminal→quantum→lease/swap/像素交T273。Node或transport double结果不冒设备GR PASS。背景前景仍新XComponent+shader，失败FAULT/PAUSED_CLOCK(NEW_RUN_REQUIRED)，无Canvas回退。
