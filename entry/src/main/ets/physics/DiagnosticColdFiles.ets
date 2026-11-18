import fs from '@ohos.file.fs';
import util from '@ohos.util';
import { utf8Bytes } from './GpuUtf8Budget';
import cryptoFramework from '@ohos.security.cryptoFramework';
import { ColdFile,TimelineStore } from './GpuBridgeTimelineCold';
// Cold files only; fsync precedes SHA receipt. Never invoked by simulation tick.
export class DiagnosticColdFiles implements TimelineStore {
 private static writerBytes:number=0;private static totalBytes:number=0;
 private static retainedBytes:number[]=[0,0,0,0,0,0];
 static retain(slot:number,n:number):boolean {
  if(!Number.isSafeInteger(slot)||!Number.isSafeInteger(n)||slot<0||slot>=6||n<0||n>16384)return false;
  const next:number=DiagnosticColdFiles.totalBytes-DiagnosticColdFiles.retainedBytes[slot]+n;
  if(next>4194304)return false;DiagnosticColdFiles.totalBytes=next;DiagnosticColdFiles.retainedBytes[slot]=n;return true;
 }
 private readonly directory:string;
 constructor(directory:string){this.directory=directory;}
 async save(name:string,text:string):Promise<ColdFile> {
  if(!/^[A-Za-z0-9_.-]+$/.test(name))throw new Error('COLD_FILENAME');
  const n:number=utf8Bytes(text);
  if(n>2097152||DiagnosticColdFiles.writerBytes+n>2097152||DiagnosticColdFiles.totalBytes+2*n>4194304)throw new Error('DIAGNOSTIC_WRITER_CREDIT');
  DiagnosticColdFiles.writerBytes+=n;DiagnosticColdFiles.totalBytes+=2*n;
  let file:fs.File|null=null;
  try{
   const bytes=new util.TextEncoder().encodeInto(text);if(bytes.length!==n)throw new Error('DIAGNOSTIC_UTF8_LENGTH');
   file=await fs.open(this.directory+'/'+name+'.part',fs.OpenMode.CREATE|fs.OpenMode.READ_WRITE|fs.OpenMode.TRUNC);
   let offset:number=0;while(offset<n){const written:number=await fs.write(file.fd,bytes.buffer.slice(offset));if(written<=0||written>n-offset)throw new Error('COLD_SHORT_WRITE');offset+=written;}
   await fs.fsync(file.fd);await fs.close(file.fd);file=null;
   const md=cryptoFramework.createMd('SHA256');await md.update({data:bytes});const digest=await md.digest();let hash='';for(let i=0;i<digest.data.length;i++)hash+=digest.data[i].toString(16).padStart(2,'0');await fs.rename(this.directory+'/'+name+'.part',this.directory+'/'+name);return {name:name,bytes:n,sha256:hash};
  }finally{
   try{if(file!==null)await fs.close(file.fd);}finally{DiagnosticColdFiles.writerBytes-=n;DiagnosticColdFiles.totalBytes-=2*n;}
  }
 }
}
