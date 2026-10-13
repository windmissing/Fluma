# 在 g.step 调用处加临时日志验证重力参数（临时版）
import re
path = 'D:/hmprojects/HelloWorld/entry/src/main/ets/pages/SandCanvas.ets'
src = open(path, encoding='utf-8').read()
if 'DEBUG gx gy' not in src:
    src = src.replace(
        '    g.step(gx * 10, gy * 10);',
        '    console.info(`[ForceInvert] step gx=${gx.toFixed(2)} gy=${gy.toFixed(2)} tiltX=${this.tiltX.toFixed(1)}`);\n    g.step(gx * 10, gy * 10);  // DEBUG gx gy')
    open(path, 'w', encoding='utf-8').write(src)
    print('debug log added')
else:
    print('already added')
