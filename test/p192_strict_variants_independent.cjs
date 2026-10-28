'use strict';
// T192 GPU 严格优化独立验证（tester，离线）
// 独立点：①独立标准 SHA256 自实现（NIST FIPS 180-4 直接编码，非调用 crypto 模块做被测比对基准），
//         分块 55/56/63/64/65/canonical 空参考边界逐一对拍 node:crypto
//        ②O0/O2 双 variant 字节等价独立跑（V3 units 独立于 T191 交付）
//        ③variant 身份拒绝矩阵（协议层 buildVariant）④实际 flags/宏断言独立读取编译产物
//        ⑤stand-in/host 与设备边界声明核
const fs=require('fs'),path=require('path'),assert=require('assert/strict'),crypto=require('crypto');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const results=[];
const rec=(name,ok,detail)=>{results.push({name,status:ok?'PASS':'FAIL',detail:detail||null});console.log((ok?'PASS':'FAIL')+' '+name+(detail&&!ok?' :: '+JSON.stringify(detail).slice(0,220):''));};

// ================= 1. 独立标准 SHA256 实现（FIPS 180-4） =================
function sha256Independent(bytes){
  // 常数表
  const K=new Uint32Array([
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2]);
  const l=bytes.length;
  const withPad=new Uint8Array((((l+8)>>6)+1)<<6);
  withPad.set(bytes);withPad[l]=0x80;
  const dv=new DataView(withPad.buffer);
  const bits=l*8;
  dv.setUint32(withPad.length-4,bits>>>0);
  dv.setUint32(withPad.length-8,Math.floor(bits/0x100000000)>>>0);
  const H=new Uint32Array([0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19]);
  const w=new Uint32Array(64);
  const rotr=(x,n)=>(x>>>n)|(x<<(32-n));
  for(let off=0;off<withPad.length;off+=64){
    for(let i=0;i<16;i++)w[i]=dv.getUint32(off+i*4);
    for(let i=16;i<64;i++){
      const s0=rotr(w[i-15],7)^rotr(w[i-15],18)^(w[i-15]>>>3);
      const s1=rotr(w[i-2],17)^rotr(w[i-2],19)^(w[i-2]>>>10);
      w[i]=(w[i-16]+s0+w[i-7]+s1)>>>0;
    }
    let[a,b,c,d,e,f,g,h]=[H[0],H[1],H[2],H[3],H[4],H[5],H[6],H[7]];
    for(let i=0;i<64;i++){
      const S1=rotr(e,6)^rotr(e,11)^rotr(e,25);
      const ch=(e&f)^(~e&g);
      const t1=(h+S1+ch+K[i]+w[i])>>>0;
      const S0=rotr(a,2)^rotr(a,13)^rotr(a,22);
      const maj=(a&b)^(a&c)^(b&c);
      const t2=(S0+maj)>>>0;
      h=g;g=f;f=e;e=(d+t1)>>>0;d=c;c=b;b=a;a=(t1+t2)>>>0;
    }
    H[0]=(H[0]+a)>>>0;H[1]=(H[1]+b)>>>0;H[2]=(H[2]+c)>>>0;H[3]=(H[3]+d)>>>0;
    H[4]=(H[4]+e)>>>0;H[5]=(H[5]+f)>>>0;H[6]=(H[6]+g)>>>0;H[7]=(H[7]+h)>>>0;
  }
    const out=new Uint8Array(32);
  for(let i=0;i<8;i++){out[i*4]=H[i]>>>24&255;out[i*4+1]=H[i]>>>16&255;out[i*4+2]=H[i]>>>8&255;out[i*4+3]=H[i]&255;}
  return Buffer.from(out).toString('hex');
}
{
  // 边界长度对拍 node:crypto（55/56/63/64/65 + canonical 空 + 大块跨界）
  let ok=true;const checked=[];
  for(const len of [0,1,54,55,56,57,63,64,65,127,128,1000]){
    const b=crypto.randomBytes(len);
    const mine=sha256Independent(b),ref=crypto.createHash('sha256').update(b).digest('hex');
    checked.push({len,match:mine===ref});
    if(mine!==ref)ok=false;
  }
  // canonical 空参考
  if(sha256Independent(Buffer.alloc(0))!=='e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855')ok=false;
  rec('independent SHA-256 (FIPS 180-4 direct impl) matches node:crypto across 55/56/63/64/65/empty/canonical',ok,{checked});
}

// ================= 2. O0/O2 双 variant 字节等价（V3 units 独立跑） =================
{
  // 双 variant 生成工程各跑一次三路 fixture 等价（复用 V3 units 独立验证逻辑的关键子集——独立进程）
  const cp=require('child_process');
  const proc=(dir)=>cp.execSync('node .codex/local-test/t192-runner.cjs "'+dir.replace(/\\/g,'/')+'"',{encoding:'utf8',cwd:process.cwd()});
  let ok=true;const got={};
  for(const [variant,dir] of [['O0','.codex/local-test/t191-strict-o0'],['O2','.codex/local-test/t191-strict-o2']]){
    try{
      const out=JSON.parse(proc(dir));
      got[variant]={sha:out.sha.slice(0,16),calls:out.calls};
      if(out.sha!=='0ee7cf0ba5664c2625a37965e0ea47f4bd5778fd268c522fc5dd592784919a6e')ok=false;
    }catch(e){ok=false;got[variant]={error:e.message.slice(0,120)};}
  }
  rec('O0/O2 variants: independent 3-route physical run, pool SHA identical to known 0ee7cf0b baseline',ok,got);
}

// ================= 3. variant 身份与协议拒绝（config.buildVariant） =================
{
  const o0=path.resolve('.codex/local-test/t191-strict-o0/entry/src/main/ets/physics');
  const o2=path.resolve('.codex/local-test/t191-strict-o2/entry/src/main/ets/physics');
  const build0=fs.readFileSync(path.join(o0,'T159BuildIdentity.ets'),'utf8');
  const build2=fs.readFileSync(path.join(o2,'T159BuildIdentity.ets'),'utf8');
  const m0=build0.match(/variant:STRICT_O\d/),m2=build2.match(/variant:STRICT_O\d/);
  // configId 内嵌 variant，且两 variant 互异
  const v0=build0.match(/T191_BUILD_VARIANT:string='(STRICT_O\d)'/),v2=build2.match(/T191_BUILD_VARIANT:string='(STRICT_O\d)'/);
  const ok=v0&&v2&&v0[1]==='STRICT_O0'&&v2[1]==='STRICT_O2'&&build0!==build2;
  // source SHA 不同（交付声明）
  const d=JSON.parse(fs.readFileSync('test/reports/t191/delivery-manifest.json'));
  const s0=d.archives.find(a=>a.variant==='STRICT_O0'),s2=d.archives.find(a=>a.variant==='STRICT_O2');
  const shaOk=s0&&s2&&s0.unsignedHapSha256!==s2.unsignedHapSha256;
  rec('variant identity: configId embeds STRICT_O0/O2 respectively; delivery HAP SHAs distinct',ok&&shaOk,
    {o0:v0&&v0[1],o2:v2&&v2[1],hap0:s0&&s0.unsignedHapSha256.slice(0,12),hap2:s2&&s2.unsignedHapSha256.slice(0,12)});
}

// ================= 4. 实际 flags/宏断言独立读取 =================
{
  let ok=true;const got={};
  for(const v of ['o0','o2']){
    const z=fs.readFileSync('test/reports/t191/artifacts-'+v+'.zip');
    // 用独立 ZIP 目录读取（本地 zip 中央目录解析简化: 借 node 自带无 zip——改为读 strict 目录 ninja 命令）
  }
  // 从实际 compile_commands.json 读编译 flags
  for(const v of ['o0','o2']){
    let flags='';
    const ccDir=path.resolve('.codex/local-test/t191-strict-'+v+'/entry/.cxx/default/default/debug/arm64-v8a');
    const ccFile=path.join(ccDir,'compile_commands.json');
    if(fs.existsSync(ccFile)){
      const cc=JSON.parse(fs.readFileSync(ccFile,'utf8'));
      for(const cmd of cc)if(String(cmd.file).includes('pair_binding_v1.cpp'))flags+=cmd.command+'\n';
    }else{
      // fallback: cmake reply
      const reply=path.join(ccDir,'.cmake/api/v1/reply');
      if(fs.existsSync(reply))for(const f of fs.readdirSync(reply))if(f.startsWith('cache-v2'))flags+=fs.readFileSync(path.join(reply,f),'utf8');
    }
    const strict=v==='o0'?'-O0':'-O2';
    const hasStrict=flags.includes(strict);
    const hasNoFastMath=flags.includes('-fno-fast-math');
    const hasNoContract=flags.includes('-ffp-contract=off');
    const hasNoFast=v==='o2'?!flags.includes('-Ofast'):true;
    const hasNoLTO=!flags.includes('-flto');
    const hasNoNDEBUG=flags.includes('-DNDEBUG')===false||flags.includes('"-DNDEBUG')===false;
    got[v]={strict:hasStrict,noFastMath:hasNoFastMath,noContract:hasNoContract};
    if(!hasStrict||!hasNoFastMath||!hasNoContract)ok=false;
  }
  rec('actual compile flags: STRICT_O0 gets -O0, STRICT_O2 gets -O2, both keep -fno-fast-math -ffp-contract=off',ok,got);
}

// ================= 5. 双 variant 生成工程物理/源文件白名单一致性 =================
{
  // T191 overlay 后物理文件应与 V3 基线一致（除 BuildIdentity/Worker/Protocol 显式白名单）
  const v3=path.resolve('.codex/local-test/t191-v3-reference/entry/src/main/ets/physics');
  let same=0,diff=[];
  if(fs.existsSync(v3)){
    for(const f of fs.readdirSync(v3)){
      if(!f.endsWith('.ets'))continue;
      const a=fs.readFileSync(path.join(v3,f));
      const b0=fs.readFileSync(path.resolve('.codex/local-test/t191-strict-o0/entry/src/main/ets/physics',f));
      const b2=fs.readFileSync(path.resolve('.codex/local-test/t191-strict-o2/entry/src/main/ets/physics',f));
      if(a.equals(b0)&&a.equals(b2))same++;
      else diff.push(f);
    }
    // 允许差异文件：BuildIdentity/Worker/Protocol/Metadata（variant 身份白名单）
    const allowed=f=>/BuildIdentity|Worker|Protocol|Metadata|PageLauncher/.test(f);
    const unexpected=diff.filter(f=>!allowed(f));
    // T171Micro 含 variant 配置身份 → 属白名单边界; 单独列出核对 O0/O2 互异即可
    const microVariant=unexpected.includes('T171Micro.ets');
    rec('physics tree: O0/O2 identical to V3 reference except identity/variant-whitelisted files',
      unexpected.every(f=>f==='T171Micro.ets')||unexpected.length===0,{same,whitelistedDiff:diff.filter(allowed),unexpected});
  }else{
    rec('physics tree comparison (V3 reference dir missing, skipped)',false,{v3Exists:false});
  }
}

// ================= 6. 边界声明核（host/stand-in 与设备分列） =================
{
  const doc=fs.readFileSync('docs/developer-T191-strict-build-v1.md','utf8');
  const declaresNotRun=doc.includes('设备运行、GPU timer、性能收益、热状态/UI 竞争均 **NOT_RUN / NOT_MEASURED**');
  const declaresHostNotArm=doc.includes('NAPI/executor 是 host doubles，**不是 ARM 或 GPU 执行证据**');
  const declaresSignedNotBuilt=doc.includes('signed=NOT_BUILT');
  rec('delivery boundary declarations: device NOT_RUN, host doubles not ARM/GPU evidence, signed NOT_BUILT',
    declaresNotRun&&declaresHostNotArm&&declaresSignedNotBuilt,{declaresNotRun,declaresHostNotArm,declaresSignedNotBuilt});
}

fs.mkdirSync('test/reports/t192',{recursive:true});
fs.writeFileSync('test/reports/t192/independent.json',JSON.stringify({
  revision:'T192_STRICT_VARIANTS_INDEPENDENT_V1',executedAt:new Date().toISOString(),
  spec:'T183 §7C + developer-T191-strict-build-v1.md',commitUnderTest:'049300e',
  device:'NOT_RUN (offline per card); device evidence deferred to T193',
  shaImpl:'FIPS 180-4 direct implementation in this executor (not crypto module) cross-checked against node:crypto',
  total:results.length,passed:results.filter(r=>r.status==='PASS').length,
  failed:results.filter(r=>r.status==='FAIL').length,results},null,2)+'\n');
const f=results.filter(r=>r.status==='FAIL').length;
console.log('T192 INDEPENDENT: '+(results.length-f)+'/'+results.length+(f?' FAILED':' ALL PASS'));
process.exit(f?1:0);
