export const beginEvidenceCapture:(configJson:string)=>string;
export const evidenceCaptureRequest: (requestIdDecimal:string) => string;
export const cancelEvidenceCaptureRequest: (requestIdDecimal:string) => string;
export const endEvidenceCapture:(captureIdDecimal:string)=>string;
export const evidenceCapturePage:(captureIdDecimal:string,cursor:number)=>string;
export const recordSourceAttempt:(record:string)=>string;
export const beginPageSession:()=>string;
export const endPageSession:(session:string)=>string;
export const pagePulse:(session:string,sequence:string,callbackNs:string)=>string;
export const setProgressV2:(enabled:boolean)=>string;
export const setColdTwoBatches:(enabled:boolean)=>string;
export const status:()=>string;
export const setTailProbe:(enabled:boolean)=>string;
export const setActive:(active:boolean)=>string;
export const offer:(command:ArrayBuffer|Uint32Array)=>string;
export const setAngle:(angleIndex:number)=>string;
export const newRun:()=>string;
export const diagnostics:()=>string;
export const timeline:(cursor:number)=>string;
export const configureM1:(K:number,Kw:number,features:number,revision:number)=>string;
export const setSource:(kind:number)=>string;
export const setGravityMode:(sensor:boolean)=>string;
export const setGravitySample:(rawDegrees:number,shakeDiagnostic:number,sequence:number)=>string;
export const configureOptics:(optics:Float32Array)=>string;
// T550 (problem T546 §3): cold input diagnostics — route is 'M3A'|'M3B'|'G9';
// attemptId is an optional decimal U64 string selecting this generation's
// first-failure snapshot instead of the last attempt.
export const gpuInputDiagnosticsV1:(route:string,attemptId:string)=>string;
