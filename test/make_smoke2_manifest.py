import json

manifest = {
    "calibrationProtocol": {
        "protocolId": "p011-observation3-v3",
        "protocolRevision": "preregistration-v1",
        "scheduleSha256": "7b59b2e4c4f4d87e9390cfc85cc7fb7c4c94a9233d70389cc8fadcba37b6bd46",
        "configuredThresholds": {"thetaStartDeg": 26, "thetaStopDeg": 20}
    }
}
with open('test/artifacts/p011-v3pilot3-20261021/smoke2-161246/check-manifest.json', 'w', encoding='utf-8') as f:
    json.dump(manifest, f)
print('manifest written')
