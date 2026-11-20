# T692 逐根 bank/event 的 host 只读接缝

冻结 T492 的实际 Run 在成功返回 AcceptedRoot 后、下一根复用任何资源前调用观察器。此时两类 root 的原 fence 已退休，实际 owner 已提交 bank 和 event arena。接缝位于隔离生成的 host 源树。生产编译与主 OFF/ON 使用原路径。

独立证据重放通过 `grs::m1::t692::Install` 在 owner 启动前注册一个只读 Sink，生命周期覆盖整个 owner。默认 sink 为空，直接返回。访问既有 App 权限时只做短 try-lock，释放后才发 GL 命令。每次读取前后重新核对权限、scope、nonce、期限和实际 EGL owner。撤权、失败或锁竞争均不能发布该根的原件。

Sink 的 reserve 提供预先分配的连续字节空间。先写实际接受 bank，再写实际 event arena 的 [0,eventEnd) 原字节。Root 含实际 scope、request、AcceptedRoot 原始 1024B completion、顺序、两段 GL buffer/target/offset/length/access/指针/unmap/原 GL code 和耗时。空 event 以零长度登记。完成后的 finish 只在 Result::Ok 时封存原件，其余结果保留根身份及失败段。重放调用者在 reserve 负责跨根 16MiB 分区余额，包含 completion 和元数据，耗尽返回空。接缝本身拒绝单根超过分区上限。两个回调须以有界预分配记录为基础，不得等待、重入或进行同步 I/O。

重放只增加只读 map/unmap、buffer 可见性 barrier、绑定保存与恢复。没有新 fence、flush、pulse 或公开 Capture 放行，也不修改原 GL 审核与 accepted 状态。已有 completion 的同源 map 配对、记录和两臂比较由 tester 按 T684 完成。接缝只提供实际字节及原生读回结果。

使用 `python test/t692/prepare_host.py .codex/local-test/<全新重放目录>` 生成 overlay，再从仓库根执行该目录的 `build.cmd`。两处冻结源以原字节 SHA 核对，生成锚点要求唯一，其他 C++ 实现均使用原 T492 闭包。编译收据见 `test/reports/t692/compile.json`。

MSVC host 编译与链接成功，driver 与实现的 FullM1Transport 布局区段按同一门通过，长度 1331/1331。实际 Run 对象文件含观察器符号。输出 PE 是 `.codex/local-test/t692-replay/t496.exe`，SHA256 为 `315ca2f87ec35dd978f8328a40e02f0e03dab2096f10649e25eed0e082bf3b37`。重放与两臂配对保留给 tester。
