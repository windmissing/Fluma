from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
D=ROOT/'entry/src/main/cpp/gpu_resident/g9spf'
PARTS=['schema_generated.glsl','wide_integer.glsl','state_access.glsl','origin_map.glsl','energy.glsl','activation_deposit.glsl','input_source.glsl','flick_verify.glsl','surface_flux.glsl','flight.glsl','capture.glsl','touch_launch.glsl','audit.glsl','root_schedule.glsl']
def assemble(tail=''):
 text='#version 310 es\nprecision highp int;\nlayout(local_size_x=64)in;\n'+'\n'.join((D/n).read_text(encoding='utf-8-sig') for n in PARTS)+'\n'+tail
 out=D/'pipeline.comp';out.write_text(text,encoding='utf-8');return out
if __name__=='__main__':assemble()
