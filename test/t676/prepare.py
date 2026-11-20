"""Prepare isolated same-page T676 capture variants, preserving actual XComponent."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'offline_handoff'))
from clone import clone
mode, output = sys.argv[1:]
a = mode == 'A'
source = '.codex/local-test/t663-delivery' if a else '.codex/local-test/t597-fixed'
project = clone(source, output)
ets = project / 'entry/src/main/ets'
(ets/'physics/T676Collector.ets').write_bytes(Path('test/t676/T676Collector.ets').read_bytes())
page = ets/'pages'/('GpuSandWaterExperience.ets' if a else 'GpuResidentM1.ets')
text = page.read_text(encoding='utf-8-sig')
assert 'XComponent(' in text
if a:
    body = """
    const raw = await c.query('M2_DISCOVERY', ():string => m1.m2CreateDiagnostics());
    const first = JSON.parse(raw) as M2CreateSnapshot;
    if (first.revision !== 'T462_M2_CREATE_DIAGNOSTIC_V1' || !/^[1-9][0-9]{0,19}$/.test(first.bootSerial)) throw new Error('M2_ATTEMPT_ABSENT');
    c.bind(first.bootSerial);
    for (let i:number=0;i<3;i++) {
      const log = await c.query('M2_LOG', ():string => m1.m2CreateLog(first.bootSerial,i));
      const parsed = JSON.parse(log) as M2CreateLog;
      if (parsed.state !== 'CREATE_LOG' || parsed.bootSerial !== first.bootSerial || parsed.object !== i) throw new Error('M2_LOG_SCOPE');
    }
    await c.query('M2_AFTER', ():string => m1.m2CreateDiagnostics(first.bootSerial));
"""
    anchor = "Button('导出 M2 创建原始诊断')"
else:
    body = """
    const raw = await c.query('COLD_CAPABILITIES', ():string => m1.coldCapabilities());
    const cap = JSON.parse(raw) as T676Capability;
    if (cap.revision !== 'T503_BOUNDED_COLD_V1' || !cap.scope) throw new Error('M1_SCOPE_ABSENT');
    const scope:string = JSON.stringify(cap.scope);
    c.bind(scope);
    await c.query('STATUS', ():string => m1.status());
    await c.query('RECOVERY', ():string => m1.recoveryState());
    await c.query('BOOT', ():string => m1.bootDiagnostics());
    const credit:number = c.remaining();
    if (credit < 16384) throw new Error('NOT_CAPTURED_CAPACITY');
    await c.query('EXEC_DIAG', ():string => m1.executionDiagnosticsV1(scope,credit));
    const after:string = await c.query('COLD_CAPABILITIES_AFTER', ():string => m1.coldCapabilities());
    if (JSON.stringify((JSON.parse(after) as T676Capability).scope) !== scope) throw new Error('SCOPE_CHANGED');
"""
    anchor = "Button('导出启动诊断')"
method = """
 private t676Busy:boolean=false;
 private async captureT676():Promise<void> {
  if(this.t676Busy)return;
  this.t676Busy=true;
  const c=new T676Collector();
  let state:string='RECORDED';
  try {
    c.begin(getContext(this).filesDir);
""" + body + """
  } catch(error) { state='INCOMPLETE '+String(error); }
  try { console.info('WS_EVENT t676 '+await c.finish(state)); }
  catch(error) { console.info('WS_EVENT t676 INCOMPLETE '+String(error)); }
  finally { this.t676Busy=false; }
 }
"""
pos = text.rfind('  build() {')
if pos < 0:
    pos = text.rfind(' build(){')
assert pos >= 0 and text.count(anchor)==1
text=text[:pos]+method+text[pos:]
text=text.replace(anchor,"Button('T676 同页诊断').onClick(()=>{this.captureT676();})\n  "+anchor)
text="import { T676Collector } from '../physics/T676Collector';\n"+text
if not a:
    text=text.replace('let recoveryPageSerial:number=0;', 'interface T676Capability { revision:string; scope:Object; }\nlet recoveryPageSerial:number=0;')
page.write_text(text,encoding='utf8')
