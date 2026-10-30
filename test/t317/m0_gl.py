from pathlib import Path
p=Path('test/t317/app_gl.cpp');s=p.read_text().replace('#define GRS_APP_M1\n','').replace('w[1]=2;w[2]=1;','w[1]=1;w[2]=0;').replace('T317_PRODUCTION_APP_GLES_V1','T317_M0_APP_GLES_V1');Path('test/t317/app_m0_gl.cpp').write_text(s)
s=Path('test/t317/build-gl.cmd').read_text().replace('test/t317/app_gl.cpp','test/t317/app_m0_gl.cpp').replace('entry/src/main/cpp/gpu_resident/m1/run.cpp','entry/src/main/cpp/gpu_resident/run.cpp entry/src/main/cpp/gpu_resident/resources.cpp entry/src/main/cpp/gpu_resident/product_wire.cpp entry/src/main/cpp/gpu_resident/source_transport.cpp').replace('entry/src/main/cpp/gpu_resident/m1/scene.cpp','entry/src/main/cpp/gpu_resident/scene.cpp').replace('/Fe.codex/local-test/t317/app.exe','/Fe.codex/local-test/t317/app-m0.exe')
# Avoid MSVC basename collision between M0/M1 source_transport.obj.
import re
s=re.sub(r'entry/src/main/cpp/gpu_resident/m1/\S+\.cpp\s*','',s)
Path('test/t317/build-m0-gl.cmd').write_text(s)
