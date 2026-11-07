const fs=require('node:fs'),cp=require('node:child_process');
const out='.codex/local-test/t404-parity';fs.mkdirSync(out,{recursive:true});
let source=fs.readFileSync('test/t324/partial_host.cpp','utf8');
source='#include <thread>\n'+source;
const extra=String.raw`
 {
  using Access=m1::FullM1HostAccess;
  std::vector<uint32_t> baseline;
  for(bool v2:{false,true}){
   m1::FullM1Transport gpu;assert(gpu.Create(K,Kw,id)==TransportCode::OK);
   m1::Run run(gpu);
   if(v2){m1::PlanGrantV2 baseline;baseline.id=baseline.session=1;assert(run.AdvancePlanV2(baseline,{1,0},K)==ControlCode::OK);}
   else assert(run.Advance(1,{1,0},{1,0},0,K)==ControlCode::OK);
   std::array<uint32_t,32> c{};
   c[0]=1129534023;c[1]=2;c[4]=1;c[5]=2;c[6]=c[8]=c[10]=c[14]=c[15]=c[24]=c[27]=1;
   c[12]=1;c[13]=6;c[28]=1;c[29]=7;
   double x=64.5,mass=.125;std::memcpy(c.data()+16,&x,8);std::memcpy(c.data()+20,&mass,8);
   assert(run.Offer(c)==ControlCode::OK);
   if(v2){m1::PlanGrantV2 grant;grant.id=2;grant.session=grant.sourceBoundary=1;
    grant.quanta=1;grant.carry=80;assert(run.AdvancePlanV2(grant,{2,0},K)==ControlCode::OK);
   }else assert(run.Advance(8333334,{2,0},{2,0},0,K)==ControlCode::OK);
   unsigned pumps=0;
   while(run.Tick().active&&++pumps<100000){const auto code=run.Pump();assert(code==TransportCode::OK||code==TransportCode::BUSY);if(code==TransportCode::BUSY)std::this_thread::sleep_for(std::chrono::milliseconds(1));}
   std::cout<<"V2="<<v2<<" pumps="<<pumps<<" active="<<run.Tick().active<<" accepted="<<run.Tick().accepted<<" quanta="<<run.Tick().successfulQuanta<<" planned="<<run.Tick().time.quanta<<"\n";
   assert(run.Tick().complete&&run.Tick().successfulQuanta==1&&run.Tick().accepted==2);
   assert(gpu.Freeze(true)==TransportCode::OK);std::vector<uint32_t> bank,events;
   assert(gpu.Capture(bank,events)==TransportCode::OK&&!events.empty());assert(gpu.Freeze(false)==TransportCode::OK);
   bank.insert(bank.end(),events.begin(),events.end());
   const auto& completion=gpu.LastCompletion();const auto& header=gpu.Owner().Header();
   bank.insert(bank.end(),completion.begin(),completion.end());bank.insert(bank.end(),header.begin(),header.end());
   glBindBuffer(GL_COPY_READ_BUFFER,Access::Publication(gpu,1));const auto count=Access::PublicationWords(gpu);
   const auto* pub=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,count*4,GL_MAP_READ_BIT));assert(pub);
   bank.insert(bank.end(),pub,pub+count);assert(glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE);
   if(!v2)baseline=bank;else assert(bank==baseline);
   assert(gpu.Release()==TransportCode::OK);
  }
  std::cout<<"T404 real Run AdvancePlanV2 vs V1 SOURCE+QUANTUM bank/event/completion/header/publication byte parity PASS\n";
 }
`;
const marker=' std::cout<<"{\\\"revision\\\":\\\"T329_PARTIAL_PUMP_LOOP_V1';
if(!source.includes(marker))throw Error('T404_PARITY_ANCHOR');
source=source.replace(marker,extra+marker);fs.writeFileSync(out+'/parity.cpp',source);
const original=fs.readFileSync('test/t336/build.cmd','utf8');
const vars=original.split(/\r?\n/).filter(l=>/^set (SRCS|INCS)=/.test(l));
const compile=original.split(/\r?\n/).find(l=>l.startsWith('cl ')&&l.includes('test/t324/partial_host.cpp'))
 .replace('test/t324/partial_host.cpp',out+'/parity.cpp').replaceAll('.codex/local-test/t336/',out+'/');
const batch=['@echo off','setlocal','call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul',
 'set "PATH=C:\\Users\\admin\\AppData\\Local\\Programs\\CokePLUS;%PATH%"',...vars,compile,
 'if errorlevel 1 exit /b 1','"'+out+'/partial.exe" 192 64','exit /b %errorlevel%'].join('\r\n');
fs.writeFileSync(out+'/build.cmd',batch);cp.execFileSync('cmd.exe',['/c',(out+'/build.cmd').replaceAll('/','\\')],{stdio:'inherit'});
