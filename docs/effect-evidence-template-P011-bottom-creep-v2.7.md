# P-011 主观证据模板 — bottom-creep-v2.7

## 目录命名

```text
demo/effect/P011/bottom-creep-v2.7/<UTC date>/<round>/
  EC-TT-01_run01.mp4
  EC-TT-01_run01_hilog.txt
  EC-TT-01_run01_env.json
  EC-TT-01_run01_manifest.csv
  EC-TT-01_T000.png
  EC-TT-01_T002.png
  EC-TT-02_slow_x0.25.png
  review/EC-TT-05_summary.csv
```

## 环境清单模板

```json
{
  "problem": "P-011",
  "routeId": "bottom-creep-v2.7",
  "algorithmVersion": "bottom-creep-v2.7",
  "gitCommit": "<40-char commit hash>",
  "gitStatusClean": false,
  "hapPath": ".ws-p011-v27-review-signed.hap",
  "hapSha256": "978E5A72099E1ADEBE5AAA2BD56479EECBC7EF868A813F5DEA9F4DA23108F678",
  "device": {
    "model": "<device model>",
    "osVersion": "<system version>",
    "refreshRate": "<Hz>",
    "resolution": "<width x height>",
    "brightnessPercent": 50,
    "performanceMode": "<mode>",
    "batteryOrPower": "<state>"
  },
  "build": {
    "hapName": "<hap file name>",
    "installTime": "<ISO8601 local time>",
    "entryParams": "rendermode=4&testmode=true&wspreset=thick&wssnapshot=true",
    "routeIdObservedInLog": false,
    "hooks": {
      "P011_EVENT": false,
      "P011_METRIC": false,
      "P011_BOTTOM": false
    }
  },
  "environment": {
    "backgroundApps": "closed",
    "notifications": "suppressed except recording indicator",
    "screenOrientation": "portrait",
    "inputEvidence": "hand pose in full-frame recording",
    "anglePrecision": "visual estimate only; no objective timing conclusion"
  },
  "tester": "<rater>",
  "startTime": "<ISO8601 local time>"
}
```

## 场景评分模板

| 字段 | 填写规则 |
|---|---|
| `scenario` | `EC-TT-01` 至 `EC-TT-05`。 |
| `round` | `01`、`02`；分差 `>= 1` 时追加 `03`。 |
| `dimension` | 跟手即时性 / 连续性 / 回正反向自然度 / 底层表层协调感 / 运动真实性 / 稳定流畅度，且只填该场景适用项。 |
| `score` | `0~10`；`6` 及格。 |
| `conclusion` | `PASS` / `FAIL` / `BLOCKED` / `N/A`。 |
| `evidence` | 对应录像、截图、慢放或逐帧文件名。 |

```csv
scenario,round,dimension,score,conclusion,evidence
EC-TT-01,01,hand-tracking-immediacy,,,
EC-TT-01,01,continuity,,,
EC-TT-01,01,center-and-reverse-naturalness,,,
EC-TT-01,01,motion-authenticity,,,
EC-TT-01,01,stable-smoothness,,,
```

## 逐帧检查模板

| 时间戳 | 源录像时间 | 倍速 | 帧号/索引 | 手部姿态 | 底层沙观察 | 水面/表层观察 | 是否疑似异常 | 截图 | 主观说明 |
|---|---:|---:|---:|---|---|---|---|---|---|
| | | 1 / 0.5 / 0.25 | | | | | yes/no | | |

## 硬门槛复核模板

| 编号 | 结论 | 证据 | 说明 |
|---|---|---|---|
| HG-TT-01 | `NOT_TRIGGERED` / `TRIGGERED` / `NOT_ASSESSED` | | |
| HG-TT-02 | `NOT_TRIGGERED` / `TRIGGERED` / `NOT_ASSESSED` | | |
| HG-TT-03 | `NOT_TRIGGERED` / `TRIGGERED` / `NOT_ASSESSED` | | |
| HG-TT-04 | `NOT_TRIGGERED` / `TRIGGERED` / `NOT_ASSESSED` | | |
| HG-TT-05 | `NOT_TRIGGERED` / `TRIGGERED` / `NOT_ASSESSED` | | |
| HG-TT-06 | `NOT_TRIGGERED` / `TRIGGERED` / `NOT_ASSESSED` | | |

## 异常复查命名

```text
<scenario>_anomaly_<startMs>_<endMs>.mp4
<scenario>_frame_<startMs>.png
<scenario>_frame_<startMs+33ms>.png
<scenario>_frame_<startMs+66ms>.png
```

无异常时也必须记录：`no anomaly observed`，不得留空。
