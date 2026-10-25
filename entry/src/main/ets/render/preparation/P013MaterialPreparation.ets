// T24: isolated offline preparation only. No page/controller/native imports.
export class P013MaterialVersions {
  materialRecipeVersion: string = 'P1_ALBEDO_DELTA_V1';
  slopeKernelVersion: string = 'E2_GAUSS_COL6_R18_P1_V2';
  colorPipelineVersion: string = 'LIN_SRGB_REINHARD_V1';
  textureSamplingVersion: string = 'P1_TEXEL_CANVAS_PX_V1';
  strataSamplingVersion: string = 'CONTINUOUS_COLUMN_WARP_V1';
  T_dataLayoutVersion: number = 2;
  zOriginVersion: number = 1;
}
export class P013PreparedMetadata extends P013MaterialVersions {
  materialBundleId: string = '';
  assetGeneration: number = 0;
  appearanceConfigRevision: number = 0;
  renderSeed: number = 0;
  geometryEpoch: number = 0;
  paletteHash: string = '';
  grainHash: string = '';
  strataHash: string = '';
  assetGeneratorVersion: string = '';
  assetGenerationParameters?: string;
  grainWidth: number = 256;
  grainHeight: number = 256;
  strataWidth: number = 64;
  strataHeight: number = 256;
  layers: number = 3;
  inputEncoding: string = 'SRGB8_STRAIGHT_OPAQUE';
  effectiveLambdaMaterial: number = 0;
  effectiveLambdaCanvasPx: number = 0;
  grainBaseSrgb: number[][] = [[145/255,168/255,111/255],[216/255,196/255,150/255],[181/255,182/255,131/255]];
  warpRangeCycles: number = 0;
}
export class P013PreparedBundle {
  readonly status: string = 'PREPARED_NOT_DEVICE_VALIDATED';
  readonly productEnabled: boolean = false;
  readonly metadata: P013PreparedMetadata;
  readonly grain: Uint8Array;
  readonly strata: Uint8Array;
  readonly contentFingerprint: string;
  constructor(metadata: P013PreparedMetadata, grain: Uint8Array,
    strata: Uint8Array, contentFingerprint: string) {
    this.metadata=metadata;this.grain=grain;this.strata=strata;this.contentFingerprint=contentFingerprint;
  }
}
export function p013PreparationFingerprint(bytes: Uint8Array): string {
  // Content checksum for deterministic offline assembly, not a cryptographic hash.
  let h = 2166136261;
  for (let i = 0; i < bytes.length; i++) h = Math.imul(h ^ bytes[i], 16777619) >>> 0;
  return h.toString(16).padStart(8, '0');
}
export function p013PrepareBundle(meta: P013PreparedMetadata, grain: Uint8Array | null,
  strata: Uint8Array | null): P013PreparedBundle {
  const v = new P013MaterialVersions();
  if (meta.materialRecipeVersion !== v.materialRecipeVersion || meta.slopeKernelVersion !== v.slopeKernelVersion ||
    meta.colorPipelineVersion !== v.colorPipelineVersion || meta.textureSamplingVersion !== v.textureSamplingVersion ||
    meta.strataSamplingVersion !== v.strataSamplingVersion || meta.T_dataLayoutVersion !== 2 || meta.zOriginVersion !== 1)
    throw new Error('MATERIAL_VERSION');
  if (meta.grainWidth !== 256 || meta.grainHeight !== 256 || meta.strataWidth !== 64 || meta.strataHeight !== 256 ||
    meta.layers !== 3 || meta.inputEncoding !== 'SRGB8_STRAIGHT_OPAQUE') throw new Error('MATERIAL_LAYOUT');
  if (grain === null || strata === null || grain.length !== 786432 || strata.length !== 196608)
    throw new Error('MATERIAL_MISSING_OR_LENGTH');
  if (!meta.materialBundleId || !meta.paletteHash || !meta.grainHash || !meta.strataHash || !meta.assetGeneratorVersion ||
    !Number.isSafeInteger(meta.assetGeneration) || meta.assetGeneration <= 0 ||
    !Number.isSafeInteger(meta.appearanceConfigRevision) || meta.appearanceConfigRevision < 0 ||
    !Number.isSafeInteger(meta.renderSeed) || !Number.isSafeInteger(meta.geometryEpoch) || meta.geometryEpoch < 0 ||
    !Number.isFinite(meta.effectiveLambdaMaterial) || meta.effectiveLambdaMaterial <= 0 ||
    !Number.isFinite(meta.effectiveLambdaCanvasPx) || meta.effectiveLambdaCanvasPx <= 0 ||
    !Number.isFinite(meta.warpRangeCycles) || meta.warpRangeCycles <= 0) throw new Error('MATERIAL_METADATA');
  const baseline = new P013PreparedMetadata().grainBaseSrgb;
  if (meta.grainBaseSrgb.length !== 3) throw new Error('MATERIAL_BASELINE');
  for (let k = 0; k < 3; k++) {
    if (meta.grainBaseSrgb[k].length !== 3) throw new Error('MATERIAL_BASELINE');
    for (let c = 0; c < 3; c++) if (meta.grainBaseSrgb[k][c] !== baseline[k][c]) throw new Error('MATERIAL_BASELINE');
  }
  for (let i = 3; i < grain.length; i += 4) if (grain[i] !== 255) throw new Error('MATERIAL_ALPHA');
  for (let i = 3; i < strata.length; i += 4) if (strata[i] !== 255) throw new Error('MATERIAL_ALPHA');
  // Own the bytes; a caller's staging buffer must not mutate the prepared asset.
  return new P013PreparedBundle(meta, grain.slice(), strata.slice(),
    p013PreparationFingerprint(grain) + ':' + p013PreparationFingerprint(strata));
}
export function p013Q(byte: number): number {
  if (!Number.isFinite(byte) || byte < 0 || byte > 255) throw new Error('MATERIAL_COLOR');
  const c = byte / 255;
  const l = Math.min(c <= 0.04045 ? c / 12.92 : Math.pow((c + 0.055) / 1.055, 2.4), 1 - 1/4096);
  return l / (1 - l);
}
export function p013PreparedColor(base: number[], grain: number[], grainBase: number[],
  strata: number[], c3: number, aB: number, aC: number, aC3: number, light: number,
  lightEnabled: boolean): number[] {
  if (base.length !== 3 || grain.length !== 3 || grainBase.length !== 3 || strata.length !== 3 ||
    ![aB,aC,aC3,light].every((v: number): boolean => Number.isFinite(v) && v >= 0)) throw new Error('MATERIAL_PARAMS');
  const out: number[] = [];
  for (let c = 0; c < 3; c++) {
    const deltaC = aC * (p013Q(strata[c]) - p013Q(128)) + aC3 * (p013Q(c3) - p013Q(128));
    const deltaB = aB * (p013Q(grain[c]) - p013Q(grainBase[c]));
    const r = Math.max(0, p013Q(base[c]) + deltaC + deltaB) * (lightEnabled ? light : 1);
    out.push(r / (1 + r));
  }
  return out; // linear target; hardware OETF belongs to future G0-qualified integration.
}
export function p013PreparedTexel(x: number, y: number, offset: number, warp: number,
  warpRange: number, bandMinY: number, strata: boolean): number[] {
  if (![x,y,offset,warp,warpRange,bandMinY].every((v: number): boolean => Number.isFinite(v)) || warpRange <= 0)
    throw new Error('MATERIAL_COORDINATE');
  const width = strata ? 64 : 256;
  const ix = strata ? Math.floor(warp / warpRange * 64 + 0.5) : Math.floor(x - Math.floor(offset + 0.5));
  const iy = strata ? Math.floor(y - Math.floor(bandMinY)) : Math.floor(y);
  return [((ix % width) + width) % width, ((iy % 256) + 256) % 256];
}
