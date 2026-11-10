export interface M2PackageDeclaration {packageSha256:string;libElfSha256:string;status:string;source:string;}
let initialized:boolean=false;
let declaration:M2PackageDeclaration={packageSha256:'',libElfSha256:'',status:'DECLARATION_MISSING',source:'COLD_WANT_EXTERNAL_RECEIPT'};
export function initializeM2Package(packageValue:ESObject,elfValue:ESObject):void {
 if(initialized)return;initialized=true;
 if(typeof packageValue!=='string'||typeof elfValue!=='string')return;
 const sha=packageValue as string,elf=elfValue as string;
 if(!/^[a-fA-F0-9]{64}$/.test(sha)||!/^[a-fA-F0-9]{64}$/.test(elf)){declaration.status='DECLARATION_INVALID';return;}
 declaration={packageSha256:sha.toLowerCase(),libElfSha256:elf.toLowerCase(),status:'DECLARED_PAIR_HOST_RECEIPT_REQUIRED',source:'COLD_WANT_EXTERNAL_RECEIPT'};
}
export function m2PackageDeclaration():M2PackageDeclaration{return declaration;}
