// T351 diagnostic only. Exclusive leaf times; nested inclusive timers must not
// be added to these rows. Physics never reads this clock or receipt.
export class FrameBudgetV2 {
 enabled:boolean=false;readonly capacity:number=128;readonly width:number=128;
 readonly rows:Float64Array=new Float64Array(128*128);
 readonly current:Float64Array=new Float64Array(128);
 private readonly stack:Int32Array=new Int32Array(32);
 private readonly own:Float64Array=new Float64Array(32);
 private depth:number=0;private last:number=0;private started:number=0;
 private open:boolean=false;private head:number=0;count:number=0;overwritten:number=0;invalid:number=0;
 private counters:Float64Array|null=null;private readonly before:Float64Array=new Float64Array(32);
 private readonly runs:string[]=new Array<string>(128).fill('');
 private readonly tokens:string[]=new Array<string>(128).fill('');
 private token:string='';private run:string='';clockCalls:number=0;zeroSegments:number=0;
 bind(counters:Float64Array):void{this.counters=counters;}
 private now():number{this.clockCalls++;return Date.now();}
 private settle():void{const now=this.now(),dt=now-this.last;this.last=now;if(!Number.isFinite(dt)||dt<0){this.invalid++;return;}if(dt===0)this.zeroSegments++;if(this.depth>0){const s=this.stack[this.depth-1];this.current[24+s]+=dt;this.own[this.depth-1]+=dt;}}
 begin(frame:number,rawIntervalNs:number,dt:number):void{if(!this.enabled)return;if(this.open){this.invalid++;this.end();}this.current.fill(0);this.current[0]=frame;this.current[1]=rawIntervalNs;this.current[2]=dt;this.current[20]=this.clockCalls;this.current[21]=this.zeroSegments;this.current[22]=this.invalid;this.run='';this.token='';if(this.counters!==null)this.before.set(this.counters);this.open=true;this.depth=1;this.stack[0]=0;this.own[0]=0;this.last=this.started=this.now();}
 enter(stage:number):number{if(!this.enabled||!this.open)return -1;if(stage<0||stage>=20||this.depth>=32){this.invalid++;return -1;}this.settle();this.own[this.depth]=0;this.stack[this.depth++]=stage;this.current[64+stage]++;return this.depth;}
 leave(ticket:number):void{if(ticket<0)return;if(!this.open||ticket!==this.depth){this.invalid++;return;}this.settle();const stage=this.stack[this.depth-1];this.current[44+stage]=Math.max(this.current[44+stage],this.own[this.depth-1]);this.depth--;}
 add(index:number,value:number=1):void{if(this.enabled&&this.open){if(!Number.isFinite(value)){this.invalid++;return;}if(value<0)this.invalid++;this.current[index]+=value;}}
 identity(run:string,commit:number,epoch:number,n:number,live:number,token:string=''):void{if(!this.open)return;if(run.length>256||token.length>512){this.invalid++;this.run='IDENTITY_TOO_LONG';this.token='';return;}this.run=run;this.current[3]=commit;this.current[4]=epoch;this.current[5]=n;this.current[6]=live;if(token.length>0)this.token=token;}
 outcome(code:number):void{if(this.open)this.current[15]=code;}
 end():void{if(!this.open)return;this.settle();if(this.depth!==1)this.invalid++;else this.current[44]=Math.max(this.current[44],this.own[0]);this.depth=0;this.open=false;this.current[19]=this.last-this.started;this.current[20]=this.clockCalls-this.current[20];this.current[21]=this.zeroSegments-this.current[21];this.current[22]=this.invalid-this.current[22];if(this.counters!==null)for(let i=0;i<32;i++)this.current[88+i]=this.counters[i]-this.before[i];if(this.counters!==null)this.current[14]=this.current[99];this.rows.set(this.current,this.head*this.width);this.runs[this.head]=this.run;this.tokens[this.head]=this.token;this.head=(this.head+1)%this.capacity;if(this.count<this.capacity)this.count++;else this.overwritten++;}
 exportCold():string{const rows:number[][]=[],runs:string[]=[],tokens:string[]=[];for(let i=0;i<this.count;i++){const at=(this.head-this.count+this.capacity+i)%this.capacity;rows.push(Array.from(this.rows.subarray(at*this.width,(at+1)*this.width)));runs.push(this.runs[at]);tokens.push(this.tokens[at]);}return JSON.stringify({revision:'T351_EXCLUSIVE_FRAME_BUDGET_V1',clock:'DATE_WALL_MS_INTEGER_NOT_SIMULATION_DT',maxNsPrecision:'RAW_CALLBACK_NUMBER',capacity:this.capacity,coverage:'RECENT_128_FRAMES_NOT_FULL_RUN',enabled:this.enabled,incomplete:this.overwritten>0||this.invalid>0||this.open,overwritten:this.overwritten,invalid:this.invalid,open:this.open,typedBytes:this.rows.byteLength+this.current.byteLength+this.stack.byteLength+this.before.byteLength+this.own.byteLength,metadata:'run/frame/commit/epoch/token',fields:['frame','rawCallbackIntervalNs','physicsDt','commit','epoch','N','live','Q','A','R','fillCopyBytes','consumerCopyBytes','derivedWriteBytes','attempts','rollbacks','outcome','authorityComparedBytes','coldViewFallbacks','reserved','wallMs','clockCalls','zeroSegments','invalidSegments'],stageTotalsAt:24,stageCallExclusiveMaxAt:44,stageCallsAt:64,counterDeltasAt:88,stages:['callbackTail','input','injection','step','preview','collect','hotApply','field','fillCopy','publicVerify','authorityCompare','seal','consumerCopy','sceneSetup','pack','nativeDraw','structure','exact70','strictDomain','reserved'],rows,runs,tokens});}
}
