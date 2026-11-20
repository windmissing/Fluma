"""Generate isolated primary/replay host builds from the frozen T492 input."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

base = Path(__file__).resolve().parents[2]
out = Path(sys.argv[1]).resolve()
replay = sys.argv[2] == 'replay'
assert sys.argv[2] in ('primary', 'replay')
subprocess.run([sys.executable, 'test/t692/prepare_host.py', str(out)], cwd=base, check=True)
frozen = base / '.codex/local-test/t492-m1-delivery/entry/src/main/cpp/gpu_resident'
inputs = json.loads((out/'inputs.json').read_bytes())

def edit(text, original, replacement):
    if text.count(original) != 1:
        raise ValueError(f'T703_FROZEN_ANCHOR {original}')
    return text.replace(original, replacement, 1)

def exact(name, sha):
    raw = (frozen/name).read_bytes()
    assert hashlib.sha256(raw).hexdigest() == sha, f'T703_FROZEN_SHA {name}'
    inputs['frozen'][name] = sha
    return raw.decode('utf8').replace('\r\n','\n')

if replay:
    app = (out/'app_bridge.cpp').read_text(encoding='utf8')
    app = edit(app, '  return app.AllowedLocked() && app.CurrentLocked(app.permit.epoch,app.permit.window) &&',
               '  t693w::root={};\n  uint64_t observedStamp=0;\n'
               '  const bool valid=app.AllowedLocked() && app.CurrentLocked(app.permit.epoch,app.permit.window) &&')
    app = edit(app, '      app.Stamp()<=app.permit.deadline && app.permit.nonce==m1::NonceOf(request) &&',
               '      (observedStamp=app.Stamp())<=app.permit.deadline && app.permit.nonce==m1::NonceOf(request) &&')
    app = edit(app, '      app.rootNonce.lo==root.nonce.lo && app.rootNonce.hi==root.nonce.hi;',
               '      app.rootNonce.lo==root.nonce.lo && app.rootNonce.hi==root.nonce.hi;\n'
               '  t693w::root={m1::NonceOf(request),app.permitStarted,app.permit.deadline,app.permit.revision,'
               'observedStamp,0,uint32_t(m1::RootCut::Map),request.kind,valid};\n  return valid;')
    run = (out/'run.cpp').read_text(encoding='utf8')
    run = edit(run, 'FullM1HostAccess::Observe(transport, accepted);',
               '{ t693w::ReplayScope readback; FullM1HostAccess::Observe(transport, accepted); }')
    (out/'run.cpp').write_text('#include "witness.h"\n'+run, encoding='utf8')
else:
    app = exact('app_bridge.cpp', inputs['frozen']['app_bridge.cpp'])
    (out/'run.cpp').write_text(exact('m1/run.cpp', inputs['frozen']['m1/run.cpp']), encoding='utf8')

app = edit(app, 'isolation.stamp=[](void* ptr){return static_cast<App*>(ptr)->Stamp();};', """
 isolation.stamp=[](void* ptr){
  const auto value=static_cast<App*>(ptr)->Stamp();
  if(t693w::root.nonce)t693w::root.stamp=value;
  return value;
 };""")
app = edit(app, '  const bool valid=app.PermitLocked(root,stamp);', """
  const bool valid=app.PermitLocked(root,stamp);
  t693w::root={m1::NonceOf(root),app.permitStarted,app.permit.deadline,app.permit.revision,
               stamp,0,uint32_t(cut),root.kind,valid};""")
app = edit(app, 'auto swapped=[](void* ptr){\n     auto& v=*static_cast<Swap*>(ptr);',
           'auto swapped=[](void* ptr){\n     t693w::RootScope witnessScope;\n     auto& v=*static_cast<Swap*>(ptr);')
app = edit(app, '  if(!app.PermitLocked(root,app.Stamp()))return false;', """
  const auto acceptedStamp=app.Stamp();
  if(!app.PermitLocked(root,acceptedStamp))return false;
  t693w::root={m1::NonceOf(root),app.permitStarted,app.permit.deadline,app.permit.revision,
               acceptedStamp,acceptedStamp,uint32_t(m1::RootCut::Commit),root.kind,true};""")
app = edit(app,'  app.AcceptLocked(out);',"""
  app.AcceptLocked(out);
  if(t496sink::BinaryTrace()){
   const auto sequence=t496sink::CallSeq().fetch_add(1);
   const auto& w=out.completion;
   std::string detail="acceptedRoot";
   for(const auto index:{4,5,6,7,18,19,20,21,22,23,24,25,27,33,35,51,52,53,54})
    detail+=" w"+std::to_string(index)+"="+std::to_string(w[index]);
   detail+=" hex=";
   const auto* raw=reinterpret_cast<const unsigned char*>(w.data());
   const char* digits="0123456789abcdef";
   for(size_t index=0;index<1024;++index){
    detail+=digits[raw[index]>>4];
    detail+=digits[raw[index]&15];
   }
   t496sink::EmitRecord(980,"ACCEPTED_ROOT",sequence,detail);
  }""")
(out/'app_bridge.cpp').write_text('#include "gl_sink.inc"\n'+app,encoding='utf8')

transport = {
    'full_transport.cpp': ('5c6b17903256b40f7716a0a127063c675f2a0d78e3361aa91ee2466d5306515c',
        'FullM1Transport', ['Poll(AcceptedRoot& out)', 'ContinueSubmit()', 'SubmitQuantum(const QuantumParameters& p,RootRequest r)']),
    'source_transport.cpp': ('872818b5297118e8106caccd15a582cb75f2f139f2a7afa6faf584fd03d4830f',
        'SourceCoreTransport', ['Poll(AcceptedRoot& out)', 'Submit(const std::array<uint32_t,32>& command,RootRequest r)']),
}
build = (out/'build.cmd').read_text(encoding='utf8')
if not replay:
    build = edit(build, '/DGRS_T692_HOST_READBACK ', '')
path = out.relative_to(base).as_posix()
build = edit(build, 'set INCS=', 'set INCS=/DGRS_T703_HOST_WITNESS /Itest/t693 ')
for name,(sha, owner, methods) in transport.items():
    text = exact('m1/'+name, sha)
    for method in methods:
        original = f'{owner}::{method}{{'
        text = edit(text, original, original+'\n t693w::RootScope witnessScope;')
    text = text.replace('#include "../', '#include "')
    (out/name).write_text('#include "witness.h"\n'+text,encoding='utf8')
    original = '%SRC%/gpu_resident/m1/'+name
    assert original in build
    build = build.replace(original, path+'/'+name)
(out/'build.cmd').write_text(build,encoding='utf8')
inputs['mode'] = sys.argv[2]
for name in ('test/t693/prepare_t703.py','test/t693/witness.h','test/t693/replay_sink.h','test/t496/map_shadow.h'):
    inputs['templates'][name] = hashlib.sha256((base/name).read_bytes()).hexdigest()
for name in ('app_bridge.cpp','run.cpp','build.cmd',*transport):
    inputs['generated'][name] = hashlib.sha256((out/name).read_bytes()).hexdigest()
(out/'inputs.json').write_text(json.dumps(inputs,indent=2)+'\n',encoding='utf8')
