import nativeLab from 'libgrs_resource_lab.so';
export interface ResourceLabPort538 {
 resourceLabStartV1(caseId:string,filesDir:string):string;
 resourceLabPollV1(job:string):string;resourceLabReleaseBarrierV1(job:string):string;
 resourceLabCancelV1(job:string):string;resourceLabExportV1(job:string):string;
}
export const resourceLab538:ResourceLabPort538=nativeLab as ResourceLabPort538;
