# T590 v3 · T676-A M2 cursor 采集——**未返回 API 精确锁定：LINK_PROGRAM（api=6）**

日期：2026-11-19，effector（tester 兼），真机（011124264R000337）。
被测：T590-cursor2-signed.hap（t663-delivery=T663 e33898e cursor 包+T590Cursor
设施页，inputSha 49c5f6b6→signed 6b1faa1d，sign=PASS）。
证据：`test/reports/t491/t590c/t468-create-1-1795092162916-*.json/.log`
（"导出 M2 创建原始诊断"按钮原生原件：native.json+两 object log+manifest）。

## T663 apiCursor 精确字段（决定性原件）

```
apiCursor: revision=1, majorStage=5(LINK), object=2(program handle 4),
api=6(LINK_PROGRAM), edge=1(ENTER), returned=-1, rawGl=-1,
sequence=71, beginNs=286807760186721, endNs=0, ownerTid=43807,
epoch=3, context=387373651584, bootSerial=1
current.stage=LINK, success=false, inCall=true,
inCallDurationNs=20567682969(20.6s), sealed=false
```

**ApiCode 枚举**：0=NOT_ENTERED 1=COMPILE_SHADER 2=GET_SHADER_COMPILE_STATUS
3=GET_ERROR 4=GET_SHADER_INFO_LOG_LENGTH 5=GET_SHADER_INFO_LOG
**6=LINK_PROGRAM** 7=GET_PROGRAM_LINK_STATUS 8=GET_PROGRAM_INFO_LOG_LENGTH…

## 判定

- **未返回 API 精确锁定 = glLinkProgram（object2 program handle4，ENTER 无
  EXIT，20.6s 开放）**——不再是大阶段"LINK"笼统标签，T663 cursor 达成目的
- COMPILE 阶段已正常完成（majorStage 推进到 5=LINK）；shader 编译本身成功
- 身份链完整：bootSerial=1/epoch=3/window/context/ownerTid 全字段 +
  layoutSha=6e01a7bd（与 T591 v3、a2 boot 同源）+ combinedShaderSha=7e19a455
- **T591 v3 的 LINK 40s 与本件 20.6s 为同一 API（glLinkProgram）的两次独立
  开放观测**——LINK_PROGRAM 不返回为稳定复现（非偶发）

## 回执

回 architect：glLinkProgram 单 API 不返回（两次独立 attempt：40.1s 与 20.6s，
同 handle 类别、同 shader SHA）——设备 glLinkProgram 行为为精确归因对象。
组合 T678 拆分的"四独立域"中 M2 域两件已同 API 收敛。
