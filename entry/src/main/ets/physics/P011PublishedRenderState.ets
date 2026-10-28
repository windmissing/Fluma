import { DerivedPublicationSealV2 } from './derived/P011DerivedCommitV2';
// Sole post-commit field+pool publication, independent of rendering backends.
import { WaterSandSnapshot } from './WaterSandField';
import { P011EndpointCommitKey } from './P011EndpointMetrics';
export type P011RenderPublicationKind = 'STEP' | 'EXTERNAL_ADMISSION' | 'RESET' | 'INVERT' | 'RESTORE' | 'ROUTE_CHANGE';
export type P011RenderCopyStatus = 'OK' | 'ROUTE_INACTIVE' | 'SCHEMA_UNAVAILABLE' | 'CAPACITY_MISMATCH' | 'INVALID_TOKEN';
export class P011PublishedRenderState {
  numericPolicy:string='P011_NUMERIC_V1';cacheRevision:string='P011_CACHE_V1';envelopeRevision:string='P011_ENVELOPE_V1';publicationRevision:string='P011_PUBLICATION_V1';derivedRevision=0;poolContentSerial=0;derivedCertificateId=0;derivedSeal:DerivedPublicationSealV2|null=null;
  mediumMode: string = 'WET_COLUMN_V1'; airDynamicsRevision: string = 'WET_AIR_LEGACY'; birthPlacementRevision: string = 'WET_COLUMN_V1';
  effectiveTiltDeg: number = 0;
  runId: string = ''; stateEpoch: number = 0; physicsCommitId: number = 0;
  publicationKind: P011RenderPublicationKind = 'RESET'; ledgerCommitKey: P011EndpointCommitKey | null = null;
  routeId: string | null = null; particleRouteActive: boolean = false;
  particleCapacity: number = 0; liveCount: number = 0;
  readonly field: WaterSandSnapshot;
  constructor(field: WaterSandSnapshot) { this.field = field; }
}
