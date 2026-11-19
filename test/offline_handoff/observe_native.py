"""Monitor real native argv via Windows process API while an authorized build runs."""
from pathlib import Path
import json,hashlib,psutil,time,sys
out=Path(sys.argv[1]);seconds=float(sys.argv[2]);deadline=time.monotonic()+seconds;seen=set();rows=[]
while time.monotonic()<deadline:
 for p in psutil.process_iter(['pid','name','create_time']):
  if p.info['name'].lower() not in ('clang++.exe','clang.exe','ld.lld.exe','ninja.exe'):continue
  key=(p.pid,p.info['create_time'])
  if key in seen:continue
  try:
   exe=p.exe();args=p.cmdline();cwd=p.cwd();row={'pid':p.pid,'startTime':p.info['create_time'],'launcher':exe,'launcherSHA':hashlib.sha256(Path(exe).read_bytes()).hexdigest(),'argv':args,'cwd':cwd,'observedMonotonicNs':str(time.monotonic_ns()),'origin':'OS_NATIVE_PROCESS_COMMAND_LINE_NOT_SHLEX'}
   # Preserve only explicitly selected isolated outputs, not other sessions.
   if any(tag in (cwd+' '+' '.join(args)).replace('\\','/') for tag in ['t668','t669','t670','t672']):rows.append(row)
   seen.add(key)
  except (psutil.Error,OSError):pass
 if rows:out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(rows,indent=2)+'\n')
 time.sleep(.01)
print('native argv observations',len(rows))
