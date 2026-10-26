// T55 reference executes on actual uploaded Float32 profiles. No display Canvas.
const fs=require('fs'),path=require('path'),assert=require('assert/strict'),crypto=require('crypto');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {P013MaterialDataPreparation:Prep}=require('../entry/src/main/ets/render/preparation/P013MaterialDataPreparation.ets');
const {p013PreparedColor:color,p013PreparedTexel:texel}=require('../entry/src/main/ets/render/preparation/P013MaterialPreparation.ets');
const root=path.resolve(process.argv[3]||'.codex/local-test/t55-fixtures'),reportRoot=path.resolve('test/reports/t55');
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
function bytes(file){return fs.readFileSync(path.join(root,file));}
function array(b){return new Float32Array(b.buffer,b.byteOffset,b.length/4);}
function tex(data,w,k,x,y){let off=((k*256+y)*w+x)*4;return [...data.subarray(off,off+3)];}
function sample(data,x,row){let r=Math.max(0,Math.min(127,x)),i=Math.min(Math.floor(r),126),t=r-i;return [0,1,2,3].map(j=>data[(row*128+i)*4+j]*(1-t)+data[(row*128+i+1)*4+j]*t);}
function reference(data,cfg,grain,strata,x,y){
 let qx=x+.5-16,qy=288-(y+.5)-16;if(qx<0||qy<0||qx>=128||qy>=256)return [0,0,0,0];
 if(cfg.invert){qx=128-qx;qy=256-qy;}
 const d=sample(data,qx,0),r1=sample(data,qx,1),r2=sample(data,qx,2),h=256-qy;
 if(h<0||h>=d[0])return [0,0,0,0];
 const k=h<d[2]?0:h<d[3]?1:2,canvasX=11+qx,canvasY=7+qy,gain=cfg.spark?r2[1]:1;
 const H=[145,168,111],L=[216,196,150],base=H.map((v,j)=>Math.floor(Math.max(0,Math.min(248,(k===0?v:k===1?L[j]:L[j]*(1-r1[1])+v*r1[1])*gain))+.5));
 const warp=k===0?r1[2]:k===1?r1[3]:r2[0];
 const gx=texel(canvasX,canvasY,[-4.5,5.2,9.8][k],warp,.25,cfg.min[k],false),cx=texel(canvasX,canvasY,0,warp,.25,cfg.min[k],true);
 const g=tex(grain,256,k,...gx),c=tex(strata,64,k,...cx),gb=[[145,168,111],[216,196,150],[181,182,131]][k];
 let c3=128;const height=cfg.max[k]-cfg.min[k];if(height>.5&&canvasY>=cfg.min[k]&&canvasY<cfg.max[k])c3=Math.floor((canvasY-cfg.min[k])/(height/2))===0?206:159;
 const light=cfg.altLight?[.8,.2,1]:[-.55,-.55,.63],norm=Math.hypot(...light),pb=r1[0],n=Math.hypot(pb,1);
 const dot=(-pb*light[0]+light[2])/(n*norm),face=Math.max(.8,Math.min(1.12,.85+.15*Math.max(0,dot)));
 const lm=cfg.on?1+Math.exp(-(d[0]-h)/24)*(face-1):1;
 const out=color(base,g,gb,c,c3,cfg.b?.75:0,cfg.c?.5:0,cfg.c?.1:0,lm,!!cfg.on);
 return [...out.map(v=>255*(v<=.0031308?12.92*v:1.055*Math.pow(v,1/2.4)-.055)),255];
}
if(process.argv[2]==='prepare'){
 fs.mkdirSync(root,{recursive:true});const src=path.resolve('.codex/local-test/t55-assets'),meta=JSON.parse(fs.readFileSync(path.join(src,'metadata.json'),'utf8'));
 const grain=Buffer.concat(['H','L','M'].map(k=>fs.readFileSync(path.join(src,'grain'+k+'.rgba')))),strata=Buffer.concat(['H','L','M'].map(k=>fs.readFileSync(path.join(src,'strata'+k+'.rgba'))));
 assert.equal(hash(grain),meta.grainHash);assert.equal(hash(strata),meta.strataHash);fs.writeFileSync(path.join(root,'grain.rgba'),grain);fs.writeFileSync(path.join(root,'strata.rgba'),strata);
 const stressG=Buffer.alloc(786432),stressC=Buffer.alloc(196608);for(let i=0;i<stressG.length;i+=4){const v=((i/4)%256)<128?0:255;stressG[i]=stressG[i+1]=stressG[i+2]=v;stressG[i+3]=255;}for(let i=0;i<stressC.length;i+=4){stressC[i]=stressC[i+1]=stressC[i+2]=0;stressC[i+3]=255;}
 fs.writeFileSync(path.join(root,'stress-grain.rgba'),stressG);fs.writeFileSync(path.join(root,'stress-strata.rgba'),stressC);
 const fixtures=[];
 function add(shape,on,b,c,spark,invert=0,stress=0,altLight=0){const seed={cols:128,capacity:256,bh:new Float64Array(128),bl:new Float64Array(128),mh:new Float64Array(128),ml:new Float64Array(128),sandTop:new Float64Array(128),waterTop:new Float64Array(128).fill(256)};
 for(let x=0;x<128;x++){const top=shape==='empty'?0:shape==='thin'?.4:shape==='ramp'?100+.2*x:shape==='pulse'?120+(x===64?10:0):120;seed.bh[x]=Math.min(40,top/3);seed.bl[x]=Math.min(40,top/3);seed.mh[x]=(top-seed.bh[x]-seed.bl[x])*(.2+.6*x/127);seed.ml[x]=top-seed.bh[x]-seed.bl[x]-seed.mh[x];seed.sandTop[x]=top;}
 const wh=Float64Array.from({length:128},(_,x)=>(x/127-.5)*.25),wl=Float64Array.from(wh,v=>-v),wm=Float64Array.from(wh,v=>v*.5),sg=Float64Array.from({length:128},(_,x)=>x%9===0?1.3:1);
 const p=new Prep();p.pack(seed,128,256,wh,wl,wm,sg);const data=p.data,min=[Infinity,Infinity,Infinity],max=[-Infinity,-Infinity,-Infinity];for(let x=0;x<128;x++){let d=sample(data,x,0);const a=[0,d[2],d[3]],z=[d[2],d[3],d[0]];for(let k=0;k<3;k++){min[k]=Math.min(min[k],7+256-z[k]);max[k]=Math.max(max[k],7+256-a[k]);}}
 const name=[shape,on,b,c,spark,invert,stress,altLight].join('_');const cfg={name,shape,on,b,c,spark,invert,stress,altLight,min,max,dataHash:hash(Buffer.from(data.buffer))};fixtures.push(cfg);fs.writeFileSync(path.join(root,name+'.bin'),Buffer.from(data.buffer));
 }
 for(const on of [0,1])for(const b of [0,1])for(const c of [0,1])for(const spark of [0,1])add('flat',on,b,c,spark);
 for(const shape of ['ramp','pulse','empty','thin'])for(const on of [0,1])add(shape,on,1,1,1);
 for(const on of [0,1])add('ramp',on,1,1,1,1);
 add('flat',0,1,1,1,0,0,1); // identical OFF pixels under different light
 for(const on of [0,1])for(const b of [0,1])for(const c of [0,1])add('flat',on,b,c,0,0,1);
 fs.writeFileSync(path.join(root,'fixtures.txt'),fixtures.length+'\n'+fixtures.map(f=>[f.name,f.on,f.b,f.c,f.spark,f.invert,f.stress,f.altLight,...f.min,...f.max].join(' ')).join('\n')+'\n');
 fs.writeFileSync(path.join(root,'manifest.json'),JSON.stringify({source:'T48 actual deterministic generator Node export; no on-screen Canvas',grainHash:hash(grain),strataHash:hash(strata),stressSource:'SYNTHETIC_0_255_NEGATIVE_Q_ONLY',fixtures},null,2));
 console.log('Prepared',fixtures.length,'fixtures');
}else if(process.argv[2]==='check'){
 fs.mkdirSync(reportRoot,{recursive:true});const m=JSON.parse(bytes('manifest.json')),grain=bytes('grain.rgba'),strata=bytes('strata.rgba'),stressG=bytes('stress-grain.rgba'),stressC=bytes('stress-strata.rgba');
 const rows=[];let structuralFailures=0;
 for(const f of m.fixtures){const data=array(bytes(f.name+'.bin')),pixels=bytes(f.name+'.rgba');assert.equal(pixels.length,160*288*4);let n=0,sum=0,max=0,worst=null,alphaBad=0,transparentBad=0,nonzero=0;const bins=[0,0,0];
 for(let y=0;y<288;y++)for(let x=0;x<160;x++){const expected=reference(data,f,f.stress?stressG:grain,f.stress?stressC:strata,x,y),i=(y*160+x)*4;if(pixels[i+3]!==expected[3])alphaBad++;if(expected[3]===0){if(pixels[i]||pixels[i+1]||pixels[i+2])transparentBad++;continue;}nonzero++;for(let j=0;j<3;j++){const e=Math.abs(pixels[i+j]-expected[j]);if(e>max){max=e;worst={x,y,channel:j,cpuCode:expected[j],gpuCode:pixels[i+j]};}sum+=e*e;n++;}}
 structuralFailures+=alphaBad+transparentBad;
 const y=80,phase=[];for(let x=24;x<40;x++)phase.push([x,pixels[(y*160+x)*4],pixels[(y*160+x)*4+1],pixels[(y*160+x)*4+2]]);
 const center=(f.min[1]+f.max[1])/2,gy=Math.floor(279-center-.5),c3Pixels=[];for(let y=gy-2;y<=gy+2;y++){if(y>=0&&y<288)c3Pixels.push([40,y,...pixels.subarray((y*160+40)*4,(y*160+40)*4+3)]);}
 rows.push({fixture:f.name,shape:f.shape,on:f.on,B:f.b,C:f.c,spark:f.spark,invert:f.invert,assets:f.stress?'SYNTHETIC_Q_BOUNDARY':'T48_REAL_BYTES',frameToken:f.name,sampleStage:'PRE_SWAP_READBACK',rgbCompared:n,opaquePixels:nonzero,alphaMismatch:alphaBad,transparentRgbMismatch:transparentBad,maxCodeError:max,worstSample:worst,rmsCodeError:n?Math.sqrt(sum/n):null,numericStatus:'MEASURED_NOT_CALIBRATED',readbackHash:hash(pixels),cPhaseAdjacentPixels:phase,c3StyleBoundaryPixels:c3Pixels});
 }
 const off=bytes('flat_0_1_1_1_0_0_0.rgba'),alt=bytes('flat_0_1_1_1_0_0_1.rgba');assert.deepEqual(off,alt,'P1 OFF light invariance must be exact');
 assert.equal(structuralFailures,0,'binary alpha and non-sand zero are hard invariants');
 assert.ok(rows.find(r=>r.fixture==='flat_0_1_1_1_0_0_0').opaquePixels>0,'OFF is complete material, never transparent P0');
 const report={scope:'Actual C++ MaterialPass+MaterialGlGpu on desktop ANGLE; no device/timing/presentation claim',versions:JSON.parse(fs.readFileSync('docs/reference-P013-p1-material-recipe-v1.json')).versions,assets:{grainHash:m.grainHash,strataHash:m.strataHash},structuralStatus:'PASS',fixtureCount:rows.length,offDifferentLightByteIdentical:true,readbackTarget:process.argv[4]||'UNSPECIFIED',numericStatus:'MEASURED_NOT_CALIBRATED',maxCodeError:Math.max(...rows.map(r=>r.maxCodeError)),rows};
 fs.writeFileSync(path.join(reportRoot,'gpu-reference'+(process.argv[4]?'-'+process.argv[4]:'')+'.json'),JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify({structuralStatus:report.structuralStatus,fixtureCount:rows.length,maxCodeError:report.maxCodeError,numericStatus:report.numericStatus}));
}else throw Error('prepare|check required');
