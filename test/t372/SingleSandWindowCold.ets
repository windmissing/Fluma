import { DiagnosticColdFiles } from '../physics/DiagnosticColdFiles';
import { ColdFile } from '../physics/GpuBridgeTimelineCold';

interface WindowTargetEvidence {
  sha256?:string;
}
interface WindowManifest {
  captureStatus:string;
  session:number;
  frameSerial:number;
  requestedToken:string;
  actualToken:string;
  candidate:WindowTargetEvidence;
  back:WindowTargetEvidence;
}
export interface WindowProbeEvidence {
  manifest:WindowManifest;
  candidate?:Uint8Array;
  back?:Uint8Array;
}
interface WindowArchive {
  probe:WindowManifest;
  candidate:ColdFile|null;
  back:ColdFile|null;
}
export function parseWindowRoi(text:string):Int32Array|null {
  if(text.length>128)return null;
  const groups=text.trim().split(';');
  if(groups.length<1||groups.length>3)return null;
  const out=new Int32Array(groups.length*4);
  for(let i=0;i<groups.length;i++){
    const fields=groups[i].split(',');
    if(fields.length!==4)return null;
    for(let j=0;j<4;j++){
      const field=fields[j].trim();
      if(!/^\d+$/.test(field))return null;
      const n=Number(field);
      if(!Number.isSafeInteger(n)||n>2147483647||(j>=2&&(n<1||n>64)))return null;
      out[i*4+j]=n;
    }
  }
  return out;
}
export async function saveWindowProbe(directory:string,evidence:WindowProbeEvidence):Promise<string>{
  const store=new DiagnosticColdFiles(directory);
  const m=evidence.manifest;
  if(m.captureStatus==='CAPTURED'&&(m.requestedToken.length===0||m.requestedToken!==m.actualToken||
     evidence.candidate===undefined||evidence.back===undefined))throw new Error('WINDOW_INCOMPLETE_CURRENT');
  const id='t372-'+Date.now()+'-'+m.session+'-'+m.frameSerial;
  const candidate=evidence.candidate===undefined?null:await store.saveBytes(id+'-candidate.rgba',evidence.candidate);
  const back=evidence.back===undefined?null:await store.saveBytes(id+'-back.rgba',evidence.back);
  if((candidate!==null&&candidate.sha256!==m.candidate.sha256)||(back!==null&&back.sha256!==m.back.sha256))
    throw new Error('WINDOW_HASH_MISMATCH');
  const archive:WindowArchive={probe:m,candidate:candidate,back:back};
  const manifest=await store.save(id+'.json',JSON.stringify(archive));
  await store.save(id+'.sha256',manifest.sha256+'  '+manifest.name+'\n');
  return m.captureStatus+' '+manifest.name;
}
