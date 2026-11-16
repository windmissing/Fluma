export interface ColdControlScope {epoch:string;run:string;context:string;surface:string;session:string;resetSerial:string;permissionRevision:string;}
export interface ColdScope {pid:string;route:string;source:string;control:ColdControlScope;}
export interface ColdCapability {revision:string;traceRevision:string;payloadRevision:string;traceSupported:boolean;diagnosticBytes:number;pageBytes:number;pageWords:number;traceCapacityBytes:number;scope:ColdScope;}
export interface BoundedColdPort {
 coldCapabilities():string;
 coldTraceBeginV1(scope:string,replaceId:string):string;coldTraceStateV1(requestId:string):string;
 coldTracePageV1(traceId:string,page:number):string;coldTraceEndV1(traceId:string):string;
 executionDiagnosticsV1(scope:string,remaining:number):string;
}
export interface DiagnosticEnvelope {revision:string;state:string;dataComplete:boolean;scopeStale:boolean;reason?:string;scope:ColdScope;limitUtf8Bytes:number;}
export function coldCapability(port:BoundedColdPort):ColdCapability {
 const capability:ColdCapability=JSON.parse(port.coldCapabilities()) as ColdCapability;
 if(capability.revision!=='T503_BOUNDED_COLD_V1'||capability.traceRevision!=='T503_M2_COLD_TRACE_V1'||capability.payloadRevision!=='T445_M2_TRACE_V3'||capability.diagnosticBytes!==2097152||capability.pageBytes!==16384||capability.pageWords!==1024||capability.traceCapacityBytes!==55410688)throw new Error('UNSUPPORTED_COLD_REVISION');
 return capability;
}
