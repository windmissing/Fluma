"""Stage exact baseline native entries via Hvigor's module libs directory."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def stage(mode, output):
    subprocess.run([sys.executable, 'test/t676/prepare.py', mode, str(output)], check=True, cwd=ROOT)
    original = 'T590-cursor2' if mode == 'A' else 'T597-fixed'
    receipt = json.loads((ROOT/f'test/reports/t257/{original}.json').read_bytes())
    unsigned=Path(receipt['input']['path'])
    original_hash = sha(unsigned.read_bytes())
    anchor = receipt['input'] if original_hash == receipt['input']['sha256'] else receipt['signed']
    package=Path(anchor['path'])
    assert sha(package.read_bytes())==anchor['sha256'], 'ORIGINAL_PACKAGE_IDENTITY'
    native={}
    with zipfile.ZipFile(package) as archive:
        for member in archive.namelist():
            if not member.startswith('libs/') or not member.endswith('.so'):
                continue
            assert len(Path(member).parts)==3
            target=output/'entry'/member
            target.parent.mkdir(parents=True,exist_ok=True)
            raw=archive.read(member)
            target.write_bytes(raw)
            native[member]=sha(raw)
    assert 'libs/arm64-v8a/libgrsm1.so' in native
    profile=output/'entry/build-profile.json5'
    build=json.loads(profile.read_bytes())
    del build['buildOption']['externalNativeOptions']
    profile.write_text(json.dumps(build,indent=2)+'\n')
    page=output/'entry/src/main/ets/pages'/('GpuSandWaterExperience.ets' if mode=='A' else 'GpuResidentM1.ets')
    if mode=='A':
        edit_a(page)
    else:
        edit_b(page)
    identities={'baseline':original,'baselinePackage':anchor,'observedUnsignedSHA':original_hash,'originalUnsigned':receipt['input'],'native':native,
                'page':str(page.relative_to(output)),'pageSHA':sha(page.read_bytes())}
    (output/'t694-staging.json').write_text(json.dumps(identities,indent=2)+'\n')

def edit_a(page):
    text=page.read_text(encoding='utf8')
    text=text.replace(' private t676Busy:boolean=false;', '''  @State private t676Mount: boolean = false;
  private t676Busy:boolean=false;''')
    text=text.replace('if (this.validCapability) {\n      XComponent', 'if (this.validCapability && this.t676Mount) {\n      XComponent')
    start=text.index("    const raw = await c.query('M2_DISCOVERY'")
    end=text.index('  } catch(error)',start)
    body="""    const pre = await c.query('M2_PRE_ARM', ():string => m1.m2CreateDiagnostics());
    const initial = JSON.parse(pre) as M2CreateSnapshot;
    const baseline:string = initial.bootSerial ?? '0';
    if (!/^(0|[1-9][0-9]{0,19})$/.test(baseline)) throw new Error('M2_BASELINE_SERIAL');
    await c.handoff(JSON.stringify({preArmMs:Date.now(),baseline:baseline,mountRequested:true}));
    this.t676Mount = true;
    let selected:string = '';
    while (c.elapsed() < 55000) {
      const raw = await c.query(selected ? 'M2_LOCKED' : 'M2_DISCOVERY',
        ():string => selected ? m1.m2CreateDiagnostics(selected) : m1.m2CreateDiagnostics());
      const snap = JSON.parse(raw) as T676M2Attempt;
      const serial:string = snap.bootSerial ?? '0';
      if (!selected && /^[1-9][0-9]{0,19}$/.test(serial) &&
          (serial.length > baseline.length || (serial.length === baseline.length && serial > baseline))) {
        selected = serial;
        c.bind(selected);
      }
      if (selected && serial !== selected) throw new Error('M2_LOCKED_SCOPE_CHANGED');
      if (selected && snap.sealed === true && /^[1-9][0-9]{0,19}$/.test(snap.nativeEndNs ?? '')) {
        for(let i:number=0;i<3;i++) {
          const log = await c.query('M2_LOG', ():string => m1.m2CreateLog(selected,i));
          const row = JSON.parse(log) as M2CreateLog;
          if(row.state !== 'CREATE_LOG' || row.bootSerial!==selected || row.object!==i) throw new Error('M2_LOG_SCOPE');
        }
        const after = await c.query('M2_AFTER', ():string => m1.m2CreateDiagnostics(selected));
        if((JSON.parse(after) as T676M2Attempt).bootSerial!==selected) throw new Error('M2_ANCHOR_SCOPE');
        break;
      }
      await new Promise<void>((resolve:()=>void)=>{setTimeout(resolve,1400);});
    }
    state=selected ? 'WINDOW_RECORDED' : 'CLOSED_WITHOUT_ATTEMPT';
"""
    text=text[:start]+body+text[end:]
    text=text.replace('interface M2CreateSnapshot', 'interface T676M2Attempt { bootSerial?:string; sealed?:boolean; nativeEndNs?:string; }\ninterface M2CreateSnapshot')
    text=text.replace('if(this.t676Busy)return;', 'if(this.t676Busy||this.t676Mount||!this.validCapability)return;')
    assert text.count('this.t676Mount = true') == 1 and '@State private t676Mount' in text and 'this.validCapability && this.t676Mount' in text
    page.write_text(text,encoding='utf8')

def edit_b(page):
    text=page.read_text(encoding='utf8')
    old='if(e.type===TouchType.Down)this.model.down(p.id,p.x,p.y);'
    assert text.count(old)==1
    text=text.replace(old,'''if(e.type===TouchType.Down){
      const returned:boolean=this.model.down(p.id,p.x,p.y);
      if(this.t676Down.length<8)this.t676Down.push(JSON.stringify({pointer:p.id,
        eventTimestamp:String(e.timestamp),observedMs:Date.now(),layout:this.inputLayout537,
        pointIndex:i,count:points.length,returned:returned}));
    }''')
    text=text.replace(' private t676Busy:boolean=false;', ' private t676Down:string[]=[];\n private t676Busy:boolean=false;')
    text=text.replace("    await c.query('EXEC_DIAG'", "    await c.handoff('['+this.t676Down.join(',')+']');\n    await c.query('EXEC_DIAG'")
    page.write_text(text,encoding='utf8')

if __name__=='__main__':
    mode,path=sys.argv[1:]
    assert mode in ('A','B')
    stage(mode,Path(path).resolve())
