"""T24 offline six-layer asset package; never enables a product renderer."""
import argparse
import hashlib
import json
from pathlib import Path
VERSIONS = dict(materialRecipeVersion='P1_ALBEDO_DELTA_V1', slopeKernelVersion='E2_GAUSS_COL6_R18_P1_V2',
                colorPipelineVersion='LIN_SRGB_REINHARD_V1', textureSamplingVersion='P1_TEXEL_CANVAS_PX_V1',
                strataSamplingVersion='CONTINUOUS_COLUMN_WARP_V1', T_dataLayoutVersion=2, zOriginVersion=1)
def load_assets(root, metadata):
    """Read six explicitly mapped raw RGBA files in H/L/M order; no image synthesis."""
    root = Path(root).resolve()
    for key, expected in VERSIONS.items():
        if metadata.get(key) != expected:
            raise ValueError('MATERIAL_VERSION:' + key)
    files = metadata.get('files', {})
    result = {}
    for kind, width, expected_size in [('grain', 256, 786432), ('strata', 64, 196608)]:
        layers = []
        for phase in ['H', 'L', 'M']:
            relative = files.get(kind + phase)
            if not isinstance(relative, str):
                raise ValueError('MATERIAL_MISSING:' + kind + phase)
            path = (root / relative).resolve()
            if root not in path.parents:
                raise ValueError('MATERIAL_PATH')
            raw = path.read_bytes()
            if len(raw) != width * 256 * 4 or any(a != 255 for a in raw[3::4]):
                raise ValueError('MATERIAL_LENGTH_OR_ALPHA:' + kind + phase)
            layers.append(raw)
        payload = b''.join(layers)
        assert len(payload) == expected_size
        actual = hashlib.sha256(payload).hexdigest()
        if metadata.get(kind + 'Hash') != actual:
            raise ValueError('MATERIAL_HASH:' + kind)
        result[kind] = payload
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--metadata', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    meta = json.loads(args.metadata.read_text(encoding='utf8'))
    payloads = load_assets(args.root, meta)  # Validate every asset before writing.
    args.output.mkdir(parents=True, exist_ok=True)
    manifest = dict(status='PREPARED_NOT_DEVICE_VALIDATED', productEnabled=False, versions=VERSIONS, assets={})
    for kind, raw in payloads.items():
        (args.output / (kind + '.rgba')).write_bytes(raw)
        manifest['assets'][kind] = dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())
    (args.output / 'prepared.json').write_text(json.dumps(manifest, sort_keys=True, indent=2) + '\n', encoding='utf8')
    return 0
if __name__ == '__main__':
    raise SystemExit(main())
