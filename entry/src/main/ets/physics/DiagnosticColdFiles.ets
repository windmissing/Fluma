import fs from '@ohos.file.fs';
import util from '@ohos.util';
import cryptoFramework from '@ohos.security.cryptoFramework';
import { ColdFile,TimelineStore } from './GpuBridgeTimelineCold';
// Cold files only; fsync precedes SHA receipt. Never invoked by simulation tick.
export class DiagnosticColdFiles implements TimelineStore {
 private readonly directory:string;
 constructor(directory:string){this.directory=directory;}
 async save(name:string,text:string):Promise<ColdFile> {
  if(!/^[A-Za-z0-9_.-]+$/.test(name))throw new Error('COLD_FILENAME');
  const bytes=new util.TextEncoder().encodeInto(text);const f=fs.openSync(this.directory+'/'+name,fs.OpenMode.CREATE|fs.OpenMode.READ_WRITE|fs.OpenMode.TRUNC);
  try{let offset=0;while(offset<bytes.length){const written=fs.writeSync(f.fd,bytes.buffer.slice(offset));if(written<=0||written>bytes.length-offset)throw new Error('COLD_SHORT_WRITE');offset+=written;}fs.fsyncSync(f.fd);}finally{fs.closeSync(f);}
  const md=cryptoFramework.createMd('SHA256');await md.update({data:bytes});const digest=await md.digest();let hash='';for(let i=0;i<digest.data.length;i++)hash+=digest.data[i].toString(16).padStart(2,'0');return {name:name,bytes:bytes.length,sha256:hash};
 }
}
