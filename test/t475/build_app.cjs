const fs = require('node:fs');
const cp = require('node:child_process');
const path = require('node:path');
const project = process.argv[2];
const out = process.argv[3];
if (!project || !out || fs.existsSync(out)) throw Error('FRESH_HOST_OUTPUT_REQUIRED');
fs.mkdirSync(out, { recursive: true });
const main = project + '/entry/src/main/cpp';
let fixture = '#include <fstream>\n' + fs.readFileSync('test/t438/app_host.cpp', 'utf8').split('namespace {')[0];
const wrappers = fs.readFileSync('test/t475/gl_test.cpp', 'utf8');
fixture += wrappers.slice(wrappers.indexOf('#define PROC'), wrappers.indexOf('using namespace grs::programcache;'));
fixture += `
static uint64_t hostBegin;
static std::string hostPhase;
void Wake475(App& a, BridgeStage stage) {
 if(stage!=BridgeStage::Wake)return;
 std::lock_guard<std::mutex> lock(a.mutex);
 create468::Attempt snapshot;
 if(create468::Global().Copy(0,snapshot)&&snapshot.sealed){
  std::cerr<<"generated app Create code="<<snapshot.code<<std::endl;
  assert(snapshot.code==0);
  std::ofstream raw("${out}/native-"+hostPhase+".json");create468::Json(raw,snapshot);
  a.quit=true;
 }
 assert(programcache::Stamp()-hostBegin<120000000000ULL);
}
int main(int argc,char** argv){
 assert(argc==3);hostPhase=argv[2];programcache::packageSource="HOST_APP_"+hostPhase;assert(programcache::Store().Configure(argv[1]));
 hostBegin=programcache::Stamp();
 auto a=std::make_unique<App>(false);a->window=reinterpret_cast<void*>(1);a->hostHook=Wake475;a->loop();
 programcache::Store().Drain();
 assert(programcache::Store().Count()>=3);
 assert(programcache::Store().ReceiptFailures()==0);
 std::cout<<"T475 actual transport App Create + three program receipts PASS\\n";
}
`;
fs.writeFileSync(out + '/app.cpp', fixture);
const vars = fs.readFileSync('test/t336/build.cmd', 'utf8').split(/\r?\n/).filter(l => /^set (SRCS|INCS)=/.test(l));
const selectedRoute=process.argv[4]??'m2route';
if(!['m1','m2route'].includes(selectedRoute))throw Error('ROUTE');
if(selectedRoute==='m1'){
 const bridge=fs.readFileSync(main+'/gpu_resident/app_bridge.cpp','utf8').replaceAll('"m2route/','"m1/').replaceAll('m2route::','m1::');
 fs.writeFileSync(out+'/app_bridge.cpp',bridge);
 for(const file of fs.readdirSync(main+'/gpu_resident').filter(n=>n.endsWith('.inc'))){
  const content=fs.readFileSync(main+'/gpu_resident/'+file,'utf8');
  if(content.includes('m2route::'))fs.writeFileSync(out+'/'+file,content.replaceAll('m2route::','m1::'));
 }
}
const route = main + '/gpu_resident/'+selectedRoute;
const sources = fs.readdirSync(route).filter(n => n.endsWith('.cpp')).map(n => route + '/' + n).join(' ');
const command = [
 '@echo off', 'setlocal',
 'call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul',
 'set "PATH=C:\\Users\\admin\\AppData\\Local\\Programs\\CokePLUS;%PATH%"',
 ...vars.map(l => l.replace(/entry\/src\/main\/cpp\/gpu_resident\/m1\/[^ ]+\.cpp /g, '').replaceAll('entry/src/main/cpp', main)),
 `cl /nologo /std:c++17 /EHsc /O2 /utf-8 /DGRS_T456_BOOT /DGRS_PROGRESS_V2_EXPERIMENT %INCS% ${out}/app.cpp test/t324/gl_extra.cpp ${main}/gpu_resident/foundation.cpp %SRCS% ${sources} /Fe${out}/app.exe /Fo${out}/`,
 'if errorlevel 1 exit /b 1',
 `${out.replaceAll('/', '\\')}\\app.exe ${out}/cache cold`,
 'if errorlevel 1 exit /b 1',
 `${out.replaceAll('/', '\\')}\\app.exe ${out}/cache warm`
];
fs.writeFileSync(out + '/build.cmd', command.join('\r\n'));
cp.execFileSync('cmd.exe', ['/c', out.replaceAll('/', '\\') + '\\build.cmd'], { stdio: 'inherit', timeout: 300000 });
