"""LF template assembly identical to the product CMake asset generation."""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[2]
GPU=ROOT/'entry/src/main/cpp/gpu_resident'
def assemble(name):
    text=(GPU/'m3a'/name).read_text(encoding='utf-8-sig')
    return re.sub(r'/\* ([A-Za-z0-9_/]+\.glsl) \*/',lambda m:(GPU/m[1]).read_text(encoding='utf-8-sig'),text)
if __name__=='__main__':
    from generate_occupied_core import generate
    from generate_occupied_replay import generate as replay
    generate();replay()
    out=ROOT/'.codex/local-test/t522-solver';out.mkdir(parents=True,exist_ok=True)
    for name,target in [('source_v4.comp','source-driver.glsl'),('quantum_v4.comp','quantum-root.glsl')]:
        (out/target).write_text(assemble(name),encoding='utf-8')
