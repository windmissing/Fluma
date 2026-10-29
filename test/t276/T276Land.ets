export class T276Land {
 readonly rows:Float64Array=new Float64Array(256*10);readonly runs:string[]=new Array<string>(256).fill('');count:number=0;dropped:number=0;private start:number=0;
 begin():void{this.start=this.count;}
 add(run:string,epoch:number,substep:number,page:number,id:number,fromState:number,toState:number,mass:number,commit:number=-1):void {if(this.count===256){this.dropped++;return;}const i=this.count++,a=i*10;this.runs[i]=run;this.rows[a]=epoch;this.rows[a+1]=substep;this.rows[a+2]=commit;this.rows[a+3]=id;this.rows[a+4]=fromState;this.rows[a+5]=toState;this.rows[a+6]=mass;this.rows[a+7]=0;this.rows[a+8]=page;this.rows[a+9]=0;}
 seal(commit:number,kept:boolean):void {if(!kept){this.count=this.start;return;}for(let i=this.start;i<this.count;i++){if(this.rows[i*10+2]<0)this.rows[i*10+2]=commit;this.rows[i*10+9]=1;}}
 exportCold():string{return JSON.stringify({revision:'T276_LAND_COMMITTED_V1',coverage:this.dropped>0?'INCOMPLETE':'BOUNDED_CAPTURE',rows:this.rows.slice(0,this.count*10),runIds:this.runs.slice(0,this.count),dropped:this.dropped,fields:['epoch','substep','commit','particle','fromState','toState','bedDelta','firstCode','hotStepEpoch','sealed']});}
}



