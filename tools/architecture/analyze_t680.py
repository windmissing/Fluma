"""T680: read-only attribution of saved evidence/source; never execute product/tests."""
import json
from pathlib import Path
from analyze_t660_t662 import ROOT, record, write

ETS = '.codex/local-test/t659-delivery-source/.codex/local-test/product/entry/src/main/ets/'
ART = 'test/reports/t491/t592b/'


def read(path):
    return json.loads((ROOT / path).read_text(encoding='utf-8-sig'))


def inspect(name):
    path = ART + name
    d = read(path)
    r = d['readiness']
    a = r.get('authority')
    o = d.get('observation')
    scope = {'prepareId': r.get('prepareId'), 'pageGeneration': r.get('pageGeneration'),
             'ownerSerial': r.get('ownerSerial'), 'owner': r.get('owner'),
             'session': None, 'observationSerial': None, 'startedMonoMs': None,
             'windowId': None, 'windowGeneration': None, 'context': None}
    result = {**record(path), 'detailTruncated': r.get('detailTruncated', False),
              'observationTruncated': d.get('observationTruncated', False),
              'authorityPresent': a is not None, 'scope': scope,
              'readinessHeader': {k: v for k, v in r.items() if k != 'authority'},
              'observation': o,
              'missingFieldRule': 'null here means unavailable, never numeric zero or false'}
    if a is None:
        if o:
            scope.update({k: o.get(k) for k in ['windowId', 'windowGeneration', 'branch']})
        result['filenameSerial'] = {'value': 25, 'status': 'SHORT_LOCAL_NAME_ONLY_NOT_PAYLOAD_CONFIRMED'}
        result['unavailable'] = ['gate.session', 'gate.startedMonoMs', 'gate.observationSerial',
                                 'owner', 'snapshot', 'raster.native', 'snapshot.nativeGeometry',
                                 'packet/native dimensions', 'context', 'inputs', 'FRAME reason/time']
        return result
    s, p = a['snapshot'], a['snapshot']['raster']
    n = p['native']
    scope.update({k: a[k] for k in ['prepareId', 'pageGeneration', 'session', 'observationSerial', 'startedMonoMs']})
    scope.update({k: s[k] for k in ['windowId', 'windowGeneration', 'context']})
    result['state'] = a['state']
    result['readinessReason'] = a['readinessReason']
    result['inputs'] = a['inputs']
    result['events'] = [json.loads(x) for x in a['events']]
    result['apiCount'] = len(a['api'])
    result['diagnosticOverflow'] = a['diagnosticOverflow']
    result['snapshotScalars'] = {k: v for k, v in s.items() if k != 'raster'}
    keys = ['revision', 'generation', 'geometry', 'rasterW', 'rasterH', 'valid', 'reason',
            'observationSerial', 'didLayoutSerial', 'requestedAtLayout', 'layoutAcknowledged',
            'firstDrawSerial', 'frameAnchorNs', 'renderNode', 'native']
    result['rasterScalars'] = {k: p[k] for k in keys}
    checks = []
    for key in ['controller', 'session', 'surface', 'context', 'run']:
        checks.append({'field': key, 'actual': n[key], 'expected': 'nonempty' if key == 'run' else 'nonzero',
                       'comparisonSatisfied': bool(n[key])})
    for key, expected in [('geometry', s['geometry']), ('packetWidth', p['rasterW']),
                          ('packetHeight', p['rasterH']), ('viewportWidth', p['rasterW']),
                          ('viewportHeight', p['rasterH'])]:
        checks.append({'field': key, 'actual': n[key], 'expected': expected,
                       'comparisonSatisfied': n[key] == expected})
    result['tenNativeChecks'] = checks
    result['nativeComparisonSatisfiedCount'] = sum(x['comparisonSatisfied'] for x in checks)
    result['earlierUnsatisfiedConditions'] = [
        {'condition': 'inputs[0].focused', 'actual': a['inputs'][0]['focused'], 'expected': False,
         'baseReasonIfEvaluated': 'INPUT_EDITING_OR_FOCUSED'},
        {'condition': 'keyboardConfiguration', 'actual': s['keyboardConfiguration'],
         'expected': 'TESTER_CONFIRMED_FIXED_SYSTEM_KEYBOARD', 'baseReasonIfEvaluated': 'KEYBOARD_VISIBILITY_UNKNOWN'},
        {'condition': 'keyboardHeight', 'actual': s['keyboardHeight'], 'expected': 0,
         'baseReasonIfEvaluated': 'KEYBOARD_OCCLUDED'},
        {'condition': 'avoid[2]', 'actual': s['avoid'][2], 'expected': 'zero area',
         'baseReasonIfEvaluated': 'KEYBOARD_OCCLUDED'},
        {'condition': 'firstDrawSerial/frameAnchorNs', 'actual': [p['firstDrawSerial'], p['frameAnchorNs']],
         'expected': ['positive', 'observed'], 'rasterReasonIfEvaluated': 'RASTER_WAIT_DRAW_PAIR'},
        {'condition': 'layoutAcknowledged', 'actual': p['layoutAcknowledged'],
         'expected': 'nonzero and equal snapshot.layoutGeneration', 'snapshotLayoutGeneration': s['layoutGeneration'],
         'rasterReasonIfEvaluated': 'RASTER_WAIT_NEW_LAYOUT'}]
    result['interpretation'] = 'PREPARING_DISMISSING_UNBOUND_BEFORE_FRAME_NOT_FAILURE_WITNESS'
    return result


def main():
    files = ['docs/test-report-T592-v3.md', ART + 'v3b-field-lock.json',
             'docs/architecture-T592-raster-v1.md', 'docs/architecture-T592-raster-contract-v1.json',
             'test/t540/RasterGeometry.ets', 'test/t540/page_raster.inc', 'test/t540/overlay.py',
             'test/t426/PrepareReadinessV4.ets', 'test/t574/t574_prepare_v4_overlay.cjs',
             'test/t574/t659_owner_glue.cjs', 'tools/singlesand/prepare_owner_build.py']
    files += [ETS + p for p in ['pages/P051DerivedV2Demo.ets', 'render/RasterGeometry.ets',
              'render/PrepareReadiness.ets', 'render/PrepareReadinessV4.ets',
              'render/P013SingleSandController.ets', 'render/P013SingleSandPreparation.ets']]
    sources = [record(p) for p in files]
    text = lambda p: (ROOT / (ETS + p)).read_text(encoding='utf-8-sig')
    raster = text('render/RasterGeometry.ets')
    observe = raster[raster.index('observe(actual:'):raster.index('acknowledge(revision:')]
    key = observe[observe.index('const key='):observe.index('const changed=')]
    controller = text('render/P013SingleSandController.ets')
    page = text('pages/P051DerivedV2Demo.ets')
    v4 = text('render/PrepareReadinessV4.ets')
    anchors = {
        'nativeGeometryProducerIsArkTSPacket': 'geometry:p.geometry' in controller,
        'packUsesContextGeometryEpoch': 'p.geometry=context.geometryEpoch' in text('render/P013SingleSandPreparation.ets'),
        'nativeGeometryExcludedFromActualChangeKey': 'native.geometry' not in key,
        'nativeCopyUpdatedWithoutAdvance': 'this.plan.native=JSON.parse(JSON.stringify(native))' in observe,
        'snapshotCopiesLiveControllerAndAssignsCurrentGeometry': 'copy.geometry=geometry' in raster,
        'actualPageReadUsesLiveControllerSnapshot': 's.raster=this.raster.snapshot(g.geometry)' in page,
        'willDrawFreshReadBeforeGateObserveAndFrame': page.index('const snapshot=this.readPrepareSnapshot();', page.index('this.prepareDrawListener=')) < page.index('this.prepareGate.observe(snapshot);', page.index('this.prepareDrawListener=')) < page.index('this.prepareGate.frame(ns,now);'),
        'coldIsStateTransitionExport': 'identity!==this.prepareFileState' in page,
        'fallbackDropsAuthority': 'detailTruncated:true,state:this.state,reason:this.reason,prepareId:this.prepareId,pageGeneration:this.pageGeneration,ownerSerial:this.serialV4,firstCause:this.firstCause,firstRawError:this.firstErrorV4,lastRawError:this.lastErrorV4' in v4,
        'authorityBudget14336': 'prepareUtf8Bytes(raw)<=14336' in v4,
        'outerBudget16384': 'prepareUtf8Bytes(full)<=16384' in page,
        'templateRasterMatchesActualBytes': (ROOT / 'test/t540/RasterGeometry.ets').read_bytes() == (ROOT / (ETS + 'render/RasterGeometry.ets')).read_bytes(),
        'templateV4MatchesActualBytes': (ROOT / 'test/t426/PrepareReadinessV4.ets').read_bytes() == (ROOT / (ETS + 'render/PrepareReadinessV4.ets')).read_bytes()}
    if not all(anchors.values()):
        raise RuntimeError('SOURCE_ANCHOR_CHANGED ' + repr([k for k, v in anchors.items() if not v]))
    prep, failed = inspect('t540-PREPARING-18.json'), inspect('t540-FAILED-25.json')
    prior = read('docs/analysis-T592-raster-v1.json')
    prior_hashes = {x['path']: x['sha256'] for x in prior['sourceFiles']}
    matching = [{'path': s['path'], 'sameAsPriorAttribution': s['sha256'] == prior_hashes[s['path']]}
                for s in sources if s['path'] in prior_hashes and s['path'].startswith(ETS)]
    last_sample = next(s['last']['appMs'] for s in failed['observation']['stages'] if s['name'] == 'SAMPLE')
    analysis = {'schema': 'T680_SAVED_SCOPE_PACKET_ATTRIBUTION_V1',
                'deviceUsed': False, 'buildRun': False, 'productOrTestsRun': False,
                'method': 'parse saved JSON, hash original bytes, inspect source anchors; no product simulation',
                'sourceFiles': sources, 'sourceAnchors': anchors, 'actualSourcePriorComparison': matching,
                'artifacts': [prep, failed], 'fieldLock': read(ART + 'v3b-field-lock.json'),
                'pairing': {'status': 'NOT_ESTABLISHED',
                            'sameShortIdsAndWindow': True,
                            'failedSampleMinusPreparingStartMs': last_sample - prep['scope']['startedMonoMs'],
                            'compatibleTimesDoNotProveIdentity': True,
                            'reportClaimsAnotherRun': True,
                            'missing': ['FAILED gate session/start/observationSerial/full owner/context', 'original FAILED filename+WS_EVENT hash association', 'same-scope raster-failure snapshot'],
                            'prohibited': 'joining PREPARING 9/7 to FAILED keyboard reason or old geometry11 JPEG'},
                'verdict': {'geometryDifference': 'REAL_PACKET_GEOMETRY_DIFFERENCE_IN_EARLY_SNAPSHOT',
                            'nativeAckMissing': 'NOT_PROVEN_NOT_AN_ACK_FIELD',
                            'legalTransient': 'ALLOWED_BY_PUBLISH_THEN_PACK_ORDER_RECOVERY_NOT_CAPTURED',
                            'staleConsumer': 'NOT_PROVEN_LIVE_READ_PATH_EXISTS',
                            'persistentPublishOrDrawBug': 'NOT_PROVEN_NO_SAME_SCOPE_FAILURE_SEQUENCE',
                            'engineeringGap': 'PROVEN_COLD_TRUNCATION_DROPS_PAIRING_AUTHORITY',
                            'HOLD_strictPNG': 'NOT_REACHED_NOT_PASS'}}
    write('docs/analysis-T680-evidence-v1.json', analysis)
    print(json.dumps({'nativeComparisonsSatisfied': prep['nativeComparisonSatisfiedCount'],
                      'nativeComparisonsTotal': len(prep['tenNativeChecks']),
                      'sourceAnchors': len(anchors), 'pairing': analysis['pairing']['status'],
                      'verdict': analysis['verdict'], 'productOrTestsRun': False}, ensure_ascii=True))


if __name__ == '__main__':
    main()
