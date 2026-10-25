// Sole post-commit field+pool publication, independent of rendering backends.
import { WaterSandSnapshot } from './WaterSandField';
import { P011EndpointCommitKey } from './P011EndpointMetrics';
export type P011RenderPublicationKind = 'STEP' | 'EXTERNAL_ADMISSION' | 'RESET' | 'INVERT' | 'RESTORE' | 'ROUTE_CHANGE';
export type P011RenderCopyStatus = 'OK' | 'ROUTE_INACTIVE' | 'SCHEMA_UNAVAILABLE' | 'CAPACITY_MISMATCH' | 'INVALID_TOKEN';
export class P011PublishedRenderState {
  runId: string = ''; stateEpoch: number = 0; physicsCommitId: number = 0;
  publicationKind: P011RenderPublicationKind = 'RESET'; ledgerCommitKey: P011EndpointCommitKey | null = null;
  routeId: string | null = null; particleRouteActive: boolean = false;
  particleCapacity: number = 0; liveCount: number = 0;
  readonly field: WaterSandSnapshot;
  constructor(field: WaterSandSnapshot) { this.field = field; }
}
