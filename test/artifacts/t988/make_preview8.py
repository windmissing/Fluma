"""Produce a diagnostic-only 8×8 fixture using T980's paired physical inputs."""

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path


HERE = Path(__file__).resolve().parent


def profile8(reference):
    size, cells, side_faces, levels = 8, 64, 72, [(8, 0), (4, 64)]
    faces = 2 * side_faces
    groups = (faces + 63) // 64
    mg_cells = sum(n*n for n, _ in levels)

    def ranges(fields, lengths):
        at, result = 0, []
        for field, words in zip(fields, lengths, strict=True):
            result.append(dict(name=field['name'], wordStart=at, words=words))
            at += words
        return result, at * 4

    state, state_bytes = ranges(reference['state'], [256, side_faces, side_faces, 2*cells, 32768])
    work, work_bytes = ranges(reference['work'], [faces]*10 + [2*mg_cells]*4 + [cells]*4 + [16*groups, 4096])
    result = dict(schema='G13_WIN32_PREVIEW8_PROFILE_V1', diagnosticOnly=True, Nx=size, Ny=size,
                  Nc=cells, Nu=side_faces, Nv=faces, h=1/size, reductionGroups=groups, MGcells=mg_cells,
                  MG=[dict(Nx=n,Ny=n,packedCellStart=offset) for n, offset in levels],
                  state=state, work=work, stateBytes=state_bytes, workBytes=work_bytes,
                  renderPackBytes=4*(64+16*cells), pressureTraceSweeps=8*len(levels)+28)
    assert (state_bytes, work_bytes, result['renderPackBytes']) == (133184, 25920, 4352)
    return result


def generate(out):
    spec = importlib.util.spec_from_file_location('t980_fixtures', HERE.parent/'t980/fixtures.py')
    fixture = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(fixture)
    reference = json.loads(fixture.LAYOUT.read_bytes())['profiles'][0]
    assert (reference['Nx'], reference['Ny']) == (16, 16)
    profile = profile8(reference)
    profile_bytes = json.dumps(profile, separators=(',', ':'), sort_keys=True).encode()
    out.mkdir(parents=True, exist_ok=True)
    fixture.HERE = out
    arms = [fixture.build(profile, arm) for arm in ('circulation', 'projection', 'zero')]
    assert [arm['inputNonzeroFaces'] for arm in arms] == [112, 112, 0]
    result = dict(profile=profile, profileSHA=hashlib.sha256(profile_bytes).hexdigest(), fixtures=arms)
    (out/'fixtures.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8', newline='\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=Path, required=True)
    generate(parser.parse_args().out)
