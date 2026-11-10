import fs from '@ohos.file.fs';
import cryptoFramework from '@ohos.security.cryptoFramework';
import { ColdFile } from './GpuBridgeTimelineCold';
export class M2CreateColdFiles {
 constructor(directory:string){this.directory=directory;}
 private directory:string;
 async saveRaw(name:string,hex:string):Promise<ColdFile>{
  if(!/^[A-Za-z0-9_.-]+$/.test(name)||hex.length>8192||hex.length%2!==0||!/^[a-f0-9]*$/.test(hex))throw new Error('CREATE_RAW_FORMAT');
  const bytes=new Uint8Array(hex.length/2);
  for(let i=0;i<bytes.length;i++)bytes[i]=parseInt(hex.slice(2*i,2*i+2),16);
  const f=fs.openSync(this.directory+'/'+name,fs.OpenMode.CREATE|fs.OpenMode.READ_WRITE|fs.OpenMode.TRUNC);
  try{let offset=0;while(offset<bytes.length){const n=fs.writeSync(f.fd,bytes.buffer.slice(offset));if(n<=0||n>bytes.length-offset)throw new Error('CREATE_RAW_SHORT_WRITE');offset+=n;}fs.fsyncSync(f.fd);}finally{fs.closeSync(f);}
  const md=cryptoFramework.createMd('SHA256');await md.update({data:bytes});const digest=await md.digest();let hash='';for(let i=0;i<digest.data.length;i++)hash+=digest.data[i].toString(16).padStart(2,'0');return {name:name,bytes:bytes.length,sha256:hash};
 }
}
