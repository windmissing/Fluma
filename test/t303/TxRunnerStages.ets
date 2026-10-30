export interface TxStageIdentity {runId:string;packageSha:string;pid:number;tid:number;backend:string;arm:string;fixture:string;suite:boolean;faultPlan:string;}
export class TxRunnerStages {
 readonly rows:Float64Array=new Float64Array(1024*8);count:number=0;dropped:number=0;sequence:number=0;lastCompletedRoot:number=0;
 firstError:string='';observerError:string='';readonly identities:string[]=[];private identityIndex:number=0;
 sink:((row:string)=>void)|null=null;constructor(privateClock:()=>number){this.clock=privateClock;}private readonly clock:()=>number;
 identity(value:TxStageIdentity):void {if(this.identities.length===0)this.emit('HEADER '+JSON.stringify({revision:'T303_RUNNER_STAGES_V1',clock:'ACTIVE_MONOTONIC_MS',capacity:1024,rowWidth:8,fields:['sequence','ms','identityIndex','stage','root','code','lastCompletedRoot','dropped']}));if(this.identities.length>=16){this.dropped++;return;}this.identities.push(JSON.stringify(value));this.identityIndex=this.identities.length-1;this.emit('IDENTITY '+this.identities[this.identityIndex]);}
 error(error:string):void {if(!this.firstError)this.firstError=error.slice(0,8192);this.emit('ERROR '+JSON.stringify({firstError:this.firstError,lastCompletedRoot:this.lastCompletedRoot}));}
 private emit(text:string):void {try{this.sink?.(text+'\n');}catch(e){if(!this.observerError)this.observerError=String(e).slice(0,1024);}}
 add(stage:number,root:number=0,code:number=0):void {
  this.sequence++;if(this.count===1024){if(this.dropped++===0)this.emit('INCOMPLETE STAGE_CAPACITY');return;}const i=this.count++*8;this.rows[i]=this.sequence;this.rows[i+1]=this.clock();this.rows[i+2]=this.identityIndex;this.rows[i+3]=stage;this.rows[i+4]=root;this.rows[i+5]=code;
  if(stage===9)this.lastCompletedRoot=root;this.rows[i+6]=this.lastCompletedRoot;this.rows[i+7]=this.dropped;
  this.emit('ROW '+JSON.stringify([this.sequence,this.rows[i+1],this.identityIndex,stage,root,code,this.lastCompletedRoot,this.dropped]));
  if(stage===14)this.emit('END '+JSON.stringify({count:this.count,dropped:this.dropped,lastCompletedRoot:this.lastCompletedRoot,firstError:this.firstError,observerError:this.observerError}));
 }
 archive():string{return JSON.stringify({revision:'T303_RUNNER_STAGES_V1',dictionary:['INVALID','WANT_VALIDATED','WORKER_CREATED','COMMAND_POSTED','COMMAND_RECEIVED','NATIVE_CONFIG_ENTER','NATIVE_CONFIG_EXIT','FIXTURE_ENTER','ROOT_ENTER','ROOT_EXIT','ARCHIVE_ENCODE_ENTER','ARCHIVE_ENCODE_EXIT','FSYNC_ENTER','FSYNC_EXIT','TERMINAL','WORKER_ERROR','WORKER_EXIT'],rowWidth:8,capacity:1024,count:this.count,dropped:this.dropped,incomplete:this.dropped>0||this.observerError.length>0,lastCompletedRoot:this.lastCompletedRoot,firstError:this.firstError,observerError:this.observerError,identities:this.identities,rows:this.rows.slice(0,this.count*8)});}
}
