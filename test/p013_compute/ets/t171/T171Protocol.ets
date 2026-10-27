import { T159Config } from './T159Native';
import { T171_FIXTURES,T171Fixture } from './T171Fixtures';
import { T159_SOURCE_SHA,T159_SHADER_SHA } from './T159BuildIdentity';
export interface T171Config extends T159Config {fullDump:boolean;}
export interface T171Command {
 revision:string;blockId:string;fixtureId:string;fixtureSha256:string;route:string;steps:number;
 oracleMode:string;timingMode:string;repeatId:number;mode:string;fault:string;
 queuedUptimeMs:number;queuedWallMs:number;output:string;recoveryShaderDirectory:string;config:T171Config;
}
export interface T171Run {route:string;warmup:boolean;repeatId:number;blockId:string;}
export function t171ConfigId(fixture:T171Fixture,steps:number,oracle:string,fault:string):string{
 return 'T171_HIGH_MERGE_V3|fixture:'+fixture.fixtureSha256+'|parameters:'+JSON.stringify(fixture.parameters)+'|steps:'+steps.toString()+'|oracle:'+oracle+'|timing:COLD_AND_STEADY_SPLIT|massPackage:0.1|fault:'+fault;
}
export function fixtureFor(c:T171Command):T171Fixture {
 if(c.revision!=='T171_RUN_BLOCK_V3'||!Number.isInteger(c.steps)||c.steps<1||c.steps>32||
  !Number.isInteger(c.repeatId)||c.repeatId<0||!/^[a-zA-Z0-9_-]{1,64}$/.test(c.blockId)||
  (c.oracleMode!=='ON'&&c.oracleMode!=='OFF')||c.timingMode!=='COLD_AND_STEADY_SPLIT'||
  (c.mode!=='CORRECTNESS'&&c.mode!=='PERFORMANCE'&&c.mode!=='FAULT'&&c.mode!=='MICRO')||
  !Number.isFinite(c.queuedUptimeMs)||c.queuedUptimeMs<0||c.output.length===0)throw new Error('V3_COMMAND_INVALID');
 if((c.mode==='CORRECTNESS'||c.mode==='MICRO')&&c.oracleMode!=='ON'||c.mode==='PERFORMANCE'&&c.oracleMode!=='OFF')throw new Error('V3_ORACLE_MODE');
 if(c.config.diagnosticOracle!==(c.oracleMode==='ON')||c.config.fullDump!==(c.oracleMode==='ON')||
  c.config.sourceManifestSha256!==T159_SOURCE_SHA||!/^[0-9a-f]{64}$/.test(c.config.buildManifestSha256)||
  c.config.buildShaderSha256!==T159_SHADER_SHA[0]||c.config.queryShaderSha256!==T159_SHADER_SHA[1]||c.config.reduceShaderSha256!==T159_SHADER_SHA[2]||
  typeof c.config.configId!=='string'||c.config.configId.indexOf(c.fixtureSha256)<0)throw new Error('V3_CONFIG_IDENTITY');
 if(['GPU','T123','LEGACY_SCAN'].indexOf(c.route)<0)throw new Error('V3_ROUTE_INVALID');
 if(c.mode==='FAULT'&&(c.route!=='GPU'||c.fault==='NONE')||c.mode!=='FAULT'&&c.fault!=='NONE')throw new Error('V3_FAULT_SCOPE');
 if(['NONE','QUARANTINE_GL','OWNER_EXHAUSTION','STALE_REQUEST_HI','STALE_CONSUME_HI','SHADER_SHA_MISMATCH'].indexOf(c.fault)<0)throw new Error('V3_FAULT_KIND');
 for(let i=0;i<T171_FIXTURES.length;i++){
  const f=T171_FIXTURES[i];if(f.fixtureId===c.fixtureId&&f.fixtureSha256===c.fixtureSha256&&f.nmax===c.config.nmax){if(c.config.configId!==t171ConfigId(f,c.steps,c.oracleMode,c.fault))throw new Error('V3_PARAMETER_IDENTITY');return f;}
 }throw new Error('V3_FIXTURE_NOT_WHITELISTED');
}
export function scheduleFor(c:T171Command):T171Run[]{
 const out:T171Run[]=[];
 if(c.mode==='FAULT'){out.push({route:'GPU',warmup:false,repeatId:c.repeatId,blockId:c.blockId});if(c.fault!=='OWNER_EXHAUSTION')out.push({route:'GPU',warmup:false,repeatId:c.repeatId+1,blockId:c.blockId+'_recovery'});return out;}
 const permutations:string[][]=[['GPU','T123','LEGACY_SCAN'],['GPU','LEGACY_SCAN','T123'],['T123','GPU','LEGACY_SCAN'],['T123','LEGACY_SCAN','GPU'],['LEGACY_SCAN','GPU','T123'],['LEGACY_SCAN','T123','GPU']];
 if(c.mode==='CORRECTNESS'){for(let i=0;i<3;i++)out.push({route:permutations[0][i],warmup:false,repeatId:c.repeatId,blockId:c.blockId});return out;}
 for(let repeat=0;repeat<3;repeat++)for(let i=0;i<3;i++)out.push({route:permutations[repeat][i],warmup:true,repeatId:repeat,blockId:c.blockId+'_warm'+repeat.toString()});
 const order:number[]=[];for(let i=0;i<30;i++)order.push(i%6);let seed=13;
 for(let i=29;i>0;i--){seed=(Math.imul(seed,1664525)+1013904223)>>>0;const j=seed%(i+1),v=order[i];order[i]=order[j];order[j]=v;}
 for(let repeat=0;repeat<30;repeat++)for(let i=0;i<3;i++)out.push({route:permutations[order[repeat]][i],warmup:false,repeatId:repeat,blockId:c.blockId+'_'+repeat.toString()});
 return out;
}
