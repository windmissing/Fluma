export const DERIVED_CACHE_V2 = 'P011_LIVE_MASS_EXACT70_V2';
export const DERIVED_POLICY_V2 = 'P011_DRY_CACHE_EXACT_V2';
export const DERIVED_ENVELOPE_V2 = 'P011_DRY_ENVELOPE_ORDERED_V2';
export const DERIVED_PUBLICATION_V2 = 'P011_DERIVED_PUBLICATION_V2';
export const DERIVED_CHECKPOINT_V2 = 'P011_DERIVED_CHECKPOINT_V2';
export const DERIVED_MIGRATION_V2 = 'P011_DERIVED_MIGRATION_V2';
export enum DerivedStatus {
    OK = 0,
    INPUT_INVALID = 1001,
    ACC_NEGATIVE = 1002,
    ACC_OVERFLOW = 1003,
    CERT_STALE = 1004,
    DRY_DOMAIN_INFEASIBLE = 1005,
    PUBLICATION_DOMAIN_MISMATCH = 1006
}
export class DerivedFaultV2 {
    readonly namespace = 'P011_DERIVED_FAILURE_V2';
    code: DerivedStatus = DerivedStatus.OK;
    reason = '';
    detail = '';
    root = 0;
    cut = '';
    ordinal = 0;
    owner = 0;
    view = 0;
    branch = 0;
    policy = DERIVED_POLICY_V2;
    slot = -1;
    bucket = -1;
    limb = -1;
    borrow = 0;
    carry = 0;
    originalHotCode: number | null = null;
    originalFieldCode: number | null = null;
    outerTransportStatus = '';
    rollbackDomain = '';
    rolledBack = false;
    firstFaultFrozen = false;
    readonly oldTupleBits = new Float64Array(16);
    readonly newTupleBits = new Float64Array(16);
}
export class DerivedCountersV2 {
    readonly values = new Float64Array(32);
    add(i: number, n: number = 1): void { this.values[i] += n; }
}
// counters: buildSlot0 remove1 add2 limb3 round4 cache5 flush6 domain7 verify8 copyBytes9 mutation10 rollback11 captureBytes12 publicationBytes13 projectionBytes14.
