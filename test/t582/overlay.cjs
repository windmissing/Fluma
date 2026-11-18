const fs=require('node:fs'),path=require('node:path');
// Same matrix drives bound wrapper generation and the build identity receipt.
const matrix=[];
for(const [prefix,route] of [['m3','M3A'],['m5','M3B'],['g9','G9']]){
 const sizes={Configure:prefix==='m3'?16:prefix==='m5'?32:24,Pulse:22,Target:19,Offer:566,ColdBegin:20,ColdState:20,ColdPage:prefix==='g9'?21:22,ColdEnd:20,Session:0,Quota:0,ColdCapabilities:0};
 if(prefix==='g9'){delete sizes.Offer;Object.assign(sizes,{ResearchConfig:2,ResearchConfig64:1,Configure64:64,OfferModel:148});}
 else if(prefix==='m5')Object.assign(sizes,{ResearchConfig:2,OfferInput64:64,SourceStats:0,InputClock:0});else sizes.SourceStats=0;
 for(const [suffix,n] of Object.entries(sizes))matrix.push({api:prefix+suffix,route,n,configuration:suffix==='Configure'||suffix==='Configure64'});
}
module.exports=function(main){
 const base=path.join(main,'cpp/gpu_resident'),file=path.join(base,'app_bridge.cpp');let s=fs.readFileSync(file,'utf8').replaceAll('\r\n','\n');
 s=s.replace('napi_value v;napi_create_string_utf8(e,s.c_str(),s.size(),&v);return v;','napi_value v=nullptr;if(napi_create_string_utf8(e,s.c_str(),s.size(),&v)!=napi_ok)return nullptr;return v;');
 // Replace the actually registered diagnostic endpoint, not an unbound alias.
 const start=s.indexOf('napi_value gpuInputDiagnosticsV1(');let end=s.indexOf('{',start)+1,depth=1,quoted=false,escaped=false;
 for(;end<s.length&&depth;end++){const c=s[end];if(quoted){if(escaped)escaped=false;else if(c==='\\')escaped=true;else if(c==='"')quoted=false;}else if(c==='"')quoted=true;else if(c==='{')depth++;else if(c==='}')depth--;}
 if(start<0||end<0)throw Error('T582_DIAGNOSTIC_BINDING');
 s=s.slice(0,start)+'napi_value gpuInputDiagnosticsV1(napi_env e,napi_callback_info info){return grs::words::Diagnostics(e,info);}\n'+s.slice(end);
 const bound=[];
 for(const item of matrix){
  const pattern=new RegExp('napi_value '+item.api+'\\(napi_env');
  if(!pattern.test(s))continue;
  if(!s.includes(item.api+'Implementation582')){
   s=s.replace(pattern,'napi_value '+item.api+'Implementation582(napi_env');
   // Each definition belongs to the same M1 conditional as its implementation.
   const signature='napi_value '+item.api+'Implementation582(';let at=s.indexOf(signature),brace=s.indexOf('{',at),depth=1,i=brace+1;
   // Bodies contain escaped strings/comments; use a lexical brace scanner.
   let quote='',line=false,block=false;
   for(;i<s.length&&depth;i++){const c=s[i],next=s[i+1];if(line){if(c==='\n')line=false;continue;}if(block){if(c==='*'&&next==='/'){block=false;i++;}continue;}if(quote){if(c==='\\')i++;else if(c===quote)quote='';continue;}if(c==='/'&&next==='/'){line=true;i++;continue;}if(c==='/'&&next==='*'){block=true;i++;continue;}if(c==='"'||c==="'"){quote=c;continue;}if(c==='{')depth++;if(c==='}')depth--;}
   if(depth)throw Error('T582_BODY '+item.api);
   const wrapper='\nnapi_value '+item.api+'(napi_env e,napi_callback_info info){return grs::words::Invoke(e,info,grs::words::Route::'+item.route+',"'+item.api+'",'+item.n+','+item.api+'Implementation582,'+item.configuration+','+item.configuration+');}\n';
   s=s.slice(0,i)+wrapper+s.slice(i);
  }
  if(!s.includes('{"'+item.api+'",nullptr,'+item.api+','))throw Error('T582_PROPERTY_BINDING '+item.api);
  bound.push(item);
 }
 fs.writeFileSync(file,s);
 const types=path.join(main,'cpp/types/libgrsm1/index.d.ts');let dts=fs.readFileSync(types,'utf8');dts=dts.replace(/export const gpuInputDiagnosticsV1:[^;]+;/g,'export const gpuInputDiagnosticsV1:(route:string,attemptId:string,bootStamp:string,diagGeneration:string)=>string;');fs.writeFileSync(types,dts);
 for(const name of ['GpuInputAttempt.ets','DiagnosticColdFiles.ets','GpuUtf8Budget.ets'])fs.copyFileSync('entry/src/main/ets/physics/'+name,path.join(main,'ets/physics/'+name));
 fs.writeFileSync(path.join(main,'t582-bound-input-matrix.json'),JSON.stringify({revision:'T582_BOUND_INPUT_MATRIX_V1',entries:bound},null,2)+'\n');
};
module.exports.matrix=matrix;
