# T817 v1 · 目标 20 G13 RESET 逐调用停点真机补证

日期：2026-11-22，effector，真机（011124264R000337）。
被测：T817-probe（T811 probe 回据 0ba9bf20 核验一致→signed d42401c2，
主 bundle，映射 25e750c8 hilog 验证，pid 53577/startTick 8535146）。
证据：`test/reports/t491/t817/`（probe-reset-t812.json 44 行逐调用、
probe-cancelled.json CANCEL、v1 判定）。

## 逐调用进度序列（T812 reader 格式，新采获）

`G13_VISIBILITY_PROGRESS_V1`：capacity 192、lastSeq **44**、dropped 0、
leaseHeld=true、wall=CLOCK_MONOTONIC、cpu=CLOCK_THREAD_CPUTIME_ID。

**44 行逐调用轨迹**（关键段）：
```
seq 1  CREATED_PUBLISHED
seq 39 CheckCancel EXIT result=1
seq 40 glCreateShader ENTER / seq 41 EXIT result=2
seq 42 glShaderSource ENTER / seq 43 EXIT result=0
seq 44 glCompileShader ENTER — **无 EXIT**
```

**首个未闭合调用 = glCompileShader（seq 44 进入后从未返回）**。

## 自旋实锤（与 T770 v7 一致，现有逐调用停点）

worker 54453 **R 态自旋**：90s 内 utime 13402→36524，而进度 seq 冻结
在 44——glCompileShader 内部自旋/死循环。

## 取消退役（按卡面）

- seq 45 **CANCEL_REQUESTED** 已发布 ✔
- cancelAck=None、retirement=None、leaseHeld=true——**worker 卡在
  glCompileShader 内部，无法到达取消检查点**，取消无法解锁

## T813 备选

不冒记——探针自旋是真实停点，隔离变体未触发。

## 回执

归 developer：glCompileShader 内部自旋根因（T811 探针二进制）；取消
路径同被阻塞（编译内不查取消标志）。
