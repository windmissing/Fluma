import {TxFaultPlan} from './TxFaultPlan';
export class TxBeforeFault {
 plan:TxFaultPlan|null=null;hit:boolean=false;readonly rows:string[]=[];dropped:number=0;capture:()=>string=():string=>'NOT_CAPTURED';context:()=>string=():string=>'NOT_CAPTURED';private seen:number=0;private reading:boolean=false;private backup:string='';private backupRoot:number=0;
 begin(cut:number,root:number,scope:number,readset:string):boolean {const p=this.plan;if(p===null||p.edge!=='BEFORE'||p.cutId!==cut||p.rootId!==root||this.hit||++this.seen!==p.ordinal)return false;this.hit=true;this.record(JSON.stringify({kind:'BEFORE_INPUT',plan:p,root:root,scope:scope,traceContext:this.context(),readset:readset,raw:this.capture()}));return true;}
 end(consumer:string,originalReturn:string,readset:string):void {this.record(JSON.stringify({kind:'ORIGINAL_CHECK_RETURN',consumer:consumer,originalReturn:originalReturn,restoredReadset:readset,raw:this.capture()}));}
 applied(cut:number,readset:string):void {this.record(JSON.stringify({kind:'INJECTED_READSET',root:this.plan?.rootId??0,cut:cut,readset:readset}));}
 backupRestore(root:number,scope:number,parent:number,cut:number,code:number):void {if(this.plan===null||this.reading||(cut!==302&&cut!==303))return;this.reading=true;try{if(cut===302){this.backup=this.capture();this.backupRoot=root;}else if(this.hit&&this.backupRoot===root)this.record(JSON.stringify({kind:'ORIGINAL_BACKUP_RESTORE',root:root,scope:scope,parent:parent,originalCode:code,backupRaw:this.backup,restoredRaw:this.capture()}));}finally{this.reading=false;}}
 private record(row:string):void {if(this.rows.length===512){this.dropped++;return;}this.rows.push(row);}
}
