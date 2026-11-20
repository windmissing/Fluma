"""Read T681 diagnostics without granting reserve or capture authority."""
import json
import sys
from pathlib import Path

SCOPE = ('windowId', 'windowGeneration', 'controller', 'session', 'surface', 'context', 'run')
OWNER = ('prepareId', 'pageGeneration', 'branch', *SCOPE)

def inspect(raw):
    if len(raw) > 16384:
        raise ValueError('OUTER_UTF8_LIMIT')
    outer = json.loads(raw)
    if outer.get('revision') != 'T659_PREPARE_OWNER_VALUE_V1':
        raise ValueError('OUTER_REVISION')
    report = outer['readiness']
    if len(json.dumps(report, ensure_ascii=False, separators=(',', ':')).encode()) > 14336:
        raise ValueError('READINESS_UTF8_LIMIT')
    evidence = report['pairingEvidence']
    if evidence['revision'] != 'T680_PREPARE_PAIRING_EVIDENCE_V1':
        raise ValueError('PAIRING_REVISION')
    gate, snapshot, owner = (evidence[k] for k in ('gate', 'snapshot', 'owner'))
    frame, raster = evidence['lastFrame'], evidence.get('raster') or {}
    gs, fs = gate['observationSerial'], frame['observationSerial']
    match = None if gs is None or fs is None else gs == fs
    if frame['snapshotSerialMatches'] != match:
        raise ValueError('FRAME_SERIAL_MISMATCH')
    identity = owner is not None and all(
        (isinstance(owner.get(k), str) and bool(owner[k])) if k == 'run'
        else (type(owner.get(k)) is int and 0 < owner[k] <= 2**53-1)
        for k in OWNER
    )
    authority = report.get('authority')
    truncation = evidence['truncatedFields']
    if not isinstance(truncation, list):
        raise ValueError('TRUNCATION_FIELDS')
    joinable = bool(identity and authority and not report.get('detailTruncated')
                    and not truncation and not gate.get('diagnosticOverflow')
                    and report.get('owner') == owner
                    and report.get('ownerSerial') == evidence['ownerSerial']
                    and all(snapshot.get(k) == owner[k] for k in SCOPE)
                    and all(authority.get('snapshot', {}).get(k) == owner[k] for k in SCOPE)
                    and all(gate.get(k) == owner[k] == authority.get(k)
                            for k in ('prepareId', 'pageGeneration')))
    return {'authority': 'FULL' if authority else 'SUMMARY_ONLY',
            'exactJoinEligible': joinable,
            'serials': [gs, raster.get('observationSerial'), fs],
            'summaryBytes': len(json.dumps(evidence, ensure_ascii=False, separators=(',', ':')).encode()),
            'productPASS': False}

if __name__ == '__main__':
    print(json.dumps(inspect(Path(sys.argv[1]).read_bytes()), indent=2))
