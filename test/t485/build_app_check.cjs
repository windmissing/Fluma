const fs=require('node:fs'),Module=require('node:module'),path=require('node:path');
let source=fs.readFileSync('test/t475/build_app.cjs','utf8');
// Resolve every generated shader header from the exact project compiled into the HAP.
const actualHeaders=process.argv[2]+'/entry/.cxx/default/default/debug/arm64-v8a/gpu_resident';
if(!fs.existsSync(actualHeaders+'/m1_scene_sources.h'))throw Error('BUILD_PROJECT_FIRST');
source=source.replace('/DGRS_PROGRESS_V2_EXPERIMENT %INCS%', '/DGRS_PROGRESS_V2_EXPERIMENT /I'+actualHeaders+' %INCS%');
if(process.argv[4]==='m1'){
 source=source.replace('if(create468::Global().Copy(0,snapshot)&&snapshot.sealed){','if(a.recovery.phase==recovery467::Phase::WAIT_NEW_SESSION){')
  .replace('std::cerr<<"generated app Create code="<<snapshot.code<<std::endl;','std::cerr<<"generated M1 resources and scene prepared"<<std::endl;')
  .replace('create468::Json(raw,snapshot);','raw<<R"JSON({"revision":"T485_M1_SCENE_READY_CHECK_V1","preparedGraphics":)JSON"<<grs::programcache::graphicsRecords.size()<<R"JSON(,"ready":)JSON"<<(a.AllowedLocked()?"true":"false")<<R"JSON(,"coldQuota":")JSON"<<a.progress.generated<<R"JSON("})JSON";');
}
source=source.replace('assert(snapshot.code==0);',`assert(snapshot.code==0);
  assert(a.recovery.phase==recovery467::Phase::WAIT_NEW_SESSION);
  assert(graphicsRecords.size()==3);
  for(const auto& record:graphicsRecords)assert(record.second.prepared&&record.second.generation==a.epoch);
  assert(a.lastGoodControl.empty()&&!a.AllowedLocked());
  const auto token=a.RecoverySessionLocked(a.recovery.serial,a.epoch,a.transportIdentity.run,1);
  assert(token&&std::string(a.PagePulseLocked(token,1,a.Stamp()))=="PAGE_PULSE_ACCEPTED");
  assert(a.AllowedLocked()&&a.recovery.phase==recovery467::Phase::READY&&a.progress.generated==0);
  std::cerr<<"T485 scene prepared before READY; first real host pulse baseline; zero cold quota PASS"<<std::endl;`);
source=source.replace('a->window=reinterpret_cast<void*>(1);a->hostHook=Wake475;', 'a->window=reinterpret_cast<void*>(1);a->hostHook=Wake475;a->nextProgressV2=true;a->invalidate(nullptr,false);');
if(process.argv[4]==='m1')source=source.replace('assert(snapshot.code==0);','assert(a.v2ResourcesReady);').replace('create468::Attempt snapshot;','');
source=source.replace('120000000000ULL','600000000000ULL').replace('timeout: 300000','timeout: 650000');
if(process.argv[5]){
 const seed=path.resolve(process.argv[5]);
 const inject=`
 const seed=${JSON.stringify(seed)},target=out+'/cache/gpu-program-cache/v1';
 fs.mkdirSync(target,{recursive:true});
 for(const name of fs.readdirSync(seed).filter(n=>n.endsWith('.jsonl'))){
  const rows=fs.readFileSync(seed+'/'+name,'utf8').trim().split(/\\r?\\n/).map(JSON.parse);
  if(['SOURCE','INTERMEDIATE_QUANTUM','FINAL_QUANTUM'].includes(rows[0].role)){
   const binary=name.replace('.jsonl','.bin');fs.copyFileSync(seed+'/'+binary,target+'/'+binary);
  }
 }`;
 source=source.replace('fs.mkdirSync(out, { recursive: true });','fs.mkdirSync(out, { recursive: true });\n'+inject);
}
const all=fs.readFileSync('test/t485/gl_exports.inc','utf8').split(/\r?\n/);
const extra=all.filter(line=>line.startsWith('#define')||/glGetActiveAttrib|glGetProgramInterfaceiv|glGetProgramResourceiv|glGetAttribLocation|glGetFragDataLocation|glGetProgramResourceIndex|glIsProgram/.test(line)).join('\n');
source=source.replace("fs.writeFileSync(out + '/app.cpp', fixture);","fixture+='\\n'+("+JSON.stringify(extra)+");\nfs.writeFileSync(out + '/app.cpp', fixture);");
source=source.replace('assert(graphicsRecords.size()==3);','assert(grs::programcache::graphicsRecords.size()==3);').replace('record:graphicsRecords','record:grs::programcache::graphicsRecords');
const moduleFile=new Module(__filename);
moduleFile.paths=module.paths;
moduleFile._compile(source,__filename);
