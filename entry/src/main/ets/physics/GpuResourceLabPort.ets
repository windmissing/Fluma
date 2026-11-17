import nativeLab from 'libgrs_resource_lab.so';
export interface ResourceLabPort538 {
 resourceLabStartV1(caseId:string,filesDir:string):string;
 resourceLabPollV1(job:string):string;resourceLabReleaseBarrierV1(job:string):string;
 resourceLabCancelV1(job:string):string;resourceLabExportV1(job:string):string;
 // T565 (problem T560 §4): private-domain paged raw-byte export of the job's
 // own SaveBytes allowlist. Ids are decimal strings; every reply (including
 // errors) is complete bounded JSON.
 resourceLabFilesBeginV2(job:string):string;
 resourceLabFilesIndexV2(exportId:string,page:number):string;
 resourceLabFilePageV2(exportId:string,fileId:string,offset:string):string;
 resourceLabFilesEndV2(exportId:string):string;
}
export const resourceLab538:ResourceLabPort538=nativeLab as ResourceLabPort538;
