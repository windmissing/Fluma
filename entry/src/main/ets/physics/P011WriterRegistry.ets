// T135 isolated strict-kernel registry. Unsupported writers never enter the old route.
export type P011KernelName='SETTLE_RN_GRAPH_V1'|'TOP_SPLIT_RN_GRAPH_V1'|'LAND_RN_GRAPH_V1';
export type P011KernelStatus='SUPPORTED'|'STRICT_WRITER_UNSUPPORTED'|'CONTACT_HOLD_UNSUPPORTED';
export function p011KernelStatus(name: string, state: string): P011KernelStatus {
  if(name==='SETTLE_RN_GRAPH_V1'||name==='TOP_SPLIT_RN_GRAPH_V1')return 'SUPPORTED';
  if(name==='LAND_RN_GRAPH_V1')return state==='CONTACT_WAIT'?'SUPPORTED':'CONTACT_HOLD_UNSUPPORTED';
  return 'STRICT_WRITER_UNSUPPORTED';
}
