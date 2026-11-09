"""T413 V2 loader; T406 spatial/RGB algorithms stay unchanged and version isolated."""
import argparse
import importlib.util
import json
import math
from pathlib import Path
from PIL import Image
from capture_hold import verify_bundle
from clock import check

_path = Path(__file__).parents[1]/'t406'/'screen.py'
_module = importlib.util.spec_from_file_location('t413_legacy_color_mapping', _path)
legacy = importlib.util.module_from_spec(_module);_module.loader.exec_module(legacy)


def load(spec_path):
    verify_bundle(spec_path)
    spec = legacy.read(spec_path);root = spec_path.parent
    path = root/spec['manifest'];manifest = legacy.read(path)
    check(path.with_suffix('.sha256').read_text(encoding='utf-8-sig').split() == [legacy.digest(path.read_bytes()), path.name], 'MANIFEST_SHA')
    p, phase = manifest['probe'], manifest['phase']
    end = legacy.read(root/spec['hold']);hold = end['hold'];geometry = end['geometry']
    check(geometry == hold['geometry'] and geometry['revision'] == 'T413_GEOMETRY_V2' and geometry['visible'] is True and
          geometry['geometry'] == p['geometryEpoch'], 'FLOAT_GEOMETRY_MISSING')
    check(all(type(geometry[k]) in (int, float) and math.isfinite(geometry[k]) for k in
              ('globalX', 'globalY', 'widthVp', 'heightVp', 'vp2px')) and
          all(geometry[k] > 0 for k in ('widthVp', 'heightVp', 'vp2px')), 'FLOAT_GEOMETRY_MISSING')
    times = [int(p[k]) for k in ('candidateNs', 'backNs', 'captureEndNs', 'swapStartNs', 'swapEndNs', 'unbindEndNs')]
    check(times[0] > 0 and times == sorted(times), 'NATIVE_TIME_ORDER')
    check(spec['rotation'] == 0 and spec['screenshotScale'] == 1, 'SCREENSHOT_CONFIGURATION_UNSUPPORTED')
    png = root/spec['png'];image = Image.open(png)
    check(image.format == 'PNG' and image.mode in ('RGB', 'RGBA'), 'LOSSLESS_RGB_REQUIRED')
    check(list(image.size) == spec['screenshotSize'], 'SCREENSHOT_SIZE');image = image.convert('RGB')
    check(spec['deviceId'] and spec['toolConfiguration'], 'DEVICE_CONFIGURATION_MISSING')
    key = {'native': legacy.context(p), 'layout': spec['bounds'], 'floating': geometry,
           'screen': spec['screenshotSize'], 'rotation': spec['rotation'], 'scale': spec['screenshotScale'],
           'device': spec['deviceId'], 'tool': spec['toolConfiguration']}
    targets, total = {}, 0
    for r in p['rois']:
        x, y, w, h = (r[k] for k in ('x', 'y', 'width', 'height'))
        check(all(type(v) is int for v in (x, y, w, h)) and 0 <= x <= p['windowWidth']-w and
              0 <= y <= p['windowHeight']-h and 1 <= w <= 64 and 1 <= h <= 64 and
              r['byteOffset'] == total and r['stride'] == w*4, 'NATIVE_TILE_LAYOUT')
        total += w*h*4
    check(len(p['rois']) == 3 and 2*total <= 98304, 'READBACK_BUDGET')
    for target in ('candidate', 'back'):
        record = manifest[target];raw = (path.parent/record['name']).read_bytes()
        check(len(raw) == record['bytes'] == total and legacy.digest(raw) == record['sha256'] == p[target]['sha256'], target.upper()+'_SHA')
        pixels = {}
        for r in p['rois']:
            for yy in range(r['height']):
                for xx in range(r['width']):
                    pos = r['byteOffset']+(yy*r['width']+xx)*4
                    pixels[(r['x']+xx, r['y']+yy)] = tuple(raw[pos:pos+4])
        targets[target] = pixels
    return spec, manifest, image, key, targets


legacy.load = load


def main():
    cli = argparse.ArgumentParser(description=__doc__);sub = cli.add_subparsers(dest='action', required=True)
    for action, keys in [('calibrate', ('spec', 'output')), ('plan', ('calibration', 'cores', 'output')),
                         ('single', ('spec', 'calibration', 'plan')), ('pair', ('a', 'b', 'calibration', 'plan'))]:
        parser = sub.add_parser(action)
        for key in keys:parser.add_argument(key, type=Path)
    args = cli.parse_args()
    try:
        if args.action == 'calibrate':result = legacy.calibrate(args.spec, args.output)
        elif args.action == 'plan':result = legacy.plan(args.calibration, legacy.read(args.cores), args.output)
        elif args.action == 'single':result = legacy.single(args.spec, args.calibration, args.plan)
        else:result = legacy.pair(args.a, args.b, args.calibration, args.plan)
    except (ValueError, KeyError, OSError, TypeError, ZeroDivisionError, IndexError) as error:
        result = {'revision': 'T413_SCREEN_PAIR_V2', 'displayStatus': 'DISPLAY_NOT_JUDGABLE', 'reason': str(error)}
    print(json.dumps(result))


if __name__ == '__main__':main()
