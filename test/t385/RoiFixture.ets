import { SingleSandPacketV1 } from './P013SingleSandTypes';

export function windowFixtureKey(p:SingleSandPacketV1):string {
  return p.run+':'+p.epoch+':'+p.commit+':'+p.surface+':'+p.context+':'+p.geometry+':'+p.width+':'+p.height+':'+
    p.x+':'+p.y+':'+p.w+':'+p.h+':'+p.inverted+':'+p.displayHash+':'+p.pbHash+':'+p.particleHash;
}
class FixtureDisc {
  readonly x:number;readonly y:number;readonly radius:number;readonly air:boolean;
  constructor(x:number,y:number,radius:number,air:boolean){this.x=x;this.y=y;this.radius=radius;this.air=air;}
}
function bed(profile:Float32Array, x:number):number {
  const r=Math.min(127,Math.max(0,x)), i=Math.min(Math.floor(r),126);
  return profile[i]+(profile[i+1]-profile[i])*(r-i);
}
function free(discs:FixtureDisc[],x:number,y:number,w:number,h:number):boolean {
  for(const d of discs){
    const dx=Math.max(0,x-d.x,d.x-x-w),dy=Math.max(0,y-d.y,d.y-y-h);
    if(dx*dx+dy*dy<=(d.radius+1)*(d.radius+1))return false;
  }
  return true;
}
// Coordinates below are window pixels with a bottom-left origin. Keep a
// margin from the compositor border and from analytically visible discs.
export function chooseWindowFixture(p:SingleSandPacketV1,profile:Float32Array,xy:Float32Array,meta:Uint32Array):string {
  if(p.width<=0||p.height<=0||p.w<=0||p.h<=0||p.logicalW<=0||p.capacity<=0||!p.enabled||p.count<1)return '';
  const scale=p.w/p.logicalW,r=2*scale,margin=Math.ceil(2*scale)+1;
  const left=Math.ceil(Math.max(0,p.x)+margin),right=Math.floor(Math.min(p.width,p.x+p.w)-margin);
  const bottom=Math.ceil(Math.max(0,p.height-p.y-p.h)+margin),top=Math.floor(Math.min(p.height,p.height-p.y)-margin);
  if(right-left<16||top-bottom<16)return '';
  const discs:FixtureDisc[]=[];
  for(let i=0;i<p.count;i++){
    const x=xy[2*i],y=xy[2*i+1],state=meta[2*i]>>>8;
    if(state!==1&&state!==2)continue;
    const height=bed(profile,x);
    if(state===1&&y<height)continue;
    const cx=p.x+(p.inverted?1-x/128:x/128)*p.w;
    const cy=p.height-p.y-p.h+(p.inverted?1-y/p.capacity:y/p.capacity)*p.h;
    discs.push(new FixtureDisc(cx,cy,r,state===1&&(y-height)*p.h/p.capacity>r+2));
  }
  let air='';
  for(const d of discs){
    if(!d.air)continue;
    const cx=Math.floor(d.x),cy=Math.floor(d.y),dx=cx+.5-d.x,dy=cy+.5-d.y;
    if(dx*dx+dy*dy>=r*r*.8)continue;
    const half=Math.min(32,Math.max(2,Math.ceil(r)+2));
    if(cx-half<left||cx+half>right||cy-half<bottom||cy+half>top)continue;
    let overlap=false;
    for(const other of discs){if(other!==d&&Math.hypot(d.x-other.x,d.y-other.y)<=r+other.radius+1){overlap=true;break;}}
    if(!overlap){air=(cx-half)+','+(cy-half)+','+(2*half)+','+(2*half);break;}
  }
  if(air.length===0)return '';
  let bedRoi='',bg='';
  for(let col=0;col<128&&(bedRoi.length===0||bg.length===0);col++){
    const x=Math.max(left,Math.min(right-16,Math.floor(p.x+(col+.5)*p.w/128)-8));
    let low=Infinity,high=-Infinity;
    for(let ix=x;ix<x+16;ix++){
      const u=(ix+.5-p.x)*128/p.w;
      const y=bed(profile,p.inverted?128-u:u);
      low=Math.min(low,y);high=Math.max(high,y);
    }
    const base=p.height-p.y-p.h;
    const bedLo=p.inverted?Math.ceil(base+p.h*(1-low/p.capacity))+1:bottom;
    const bedHi=p.inverted?top:Math.floor(base+p.h*low/p.capacity)-1;
    const bgLo=p.inverted?bottom:Math.ceil(base+p.h*high/p.capacity)+1;
    const bgHi=p.inverted?Math.floor(base+p.h*(1-high/p.capacity))-1:top;
    if(bedRoi.length===0){
      const first=Math.max(bottom,bedLo),last=Math.min(top,bedHi)-16;
      for(let y=first;y<=last;y+=8){if(free(discs,x,y,16,16)){bedRoi=x+','+y+',16,16';break;}}
    }
    if(bg.length===0){
      const first=Math.max(bottom,bgLo),last=Math.min(top,bgHi)-16;
      for(let y=last;y>=first;y-=8){if(free(discs,x,y,16,16)){bg=x+','+y+',16,16';break;}}
    }
  }
  return bedRoi.length!==0&&bg.length!==0?bedRoi+';'+air+';'+bg:'';
}
