# T41 帧长度返修开发交付

2026-10-25；不使用真机。依据 T23 裁决§3，不改变512个Float32元素契约、字段校验或shader公式。

根因：生产DrawFrame把napi_get_typedarray_info的length与512比较，dual-field及CaptureProbe也同样按元素比较。OpenHarmony上游实现返回typedArray->ByteLength(vm)，而本地SDK头注释写元素数量；旧host桩沿注释返回元素数，掩盖单位错配。上游依据：https://github.com/openharmony/arkui_napi/blob/master/native_engine/native_api.cpp 。这说明可复现代码缺陷，不把它冒称为已取得新设备实值。

先改host桩为返回字节数，旧生产代码在首个有效DrawFrame断言失败（test/reports/t41-root-cause-red.log）。修复后同一真实桥测试通过：Float32Array长度512，字节2048，偏移16，底层buffer2080；native receipt精确返回2048/16/2080。511长度、2048元素过长view、错误类型、detached、范围/偏移、NaN、版本、stale与原子上传保护保留。dual数组按各自精确元素数×4比较，探针12个float按48B比较。没有兼容双单位猜测、JSON兜底或放宽校验。

生产controller和probe首帧/拒绝帧输出p013_tdata_boundary：JS length/byteLength/byteOffset与native读取byteLength/offset/bufferBytes/errorStage。native receipt及固定环另透出原始byte值；检查未到buffer查询阶段时0表示尚未读取，不能当实测空buffer。T42须关联同frameId采集实际设备值，再确认uploadedFrameId/T_dataHash/draw/swap与G0合成色彩，不能仅依据host通过宣称G0通过。

验证：P0 20/20、composition6/6、dualfield9/9；真实native桥与生命周期host通过；ArkTS及arm64 BuildNativeWithNinja编译通过。构建test/reports/t41-t24-build.log，自验t41-developer-selfcheck.json。最终GPU呈现与G1裁决留给T42/designer；未连接设备。
