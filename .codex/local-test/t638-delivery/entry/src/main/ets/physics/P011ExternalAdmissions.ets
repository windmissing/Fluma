// T39/O-2: independent successful-receipt accumulator, never seeded from epoch.
// O(1) retained summary. Raw records are synchronously exported, not queued.
export class P011ExternalAdmissionRecord {
  runId: string = '';
  admissionTransactionId: number = 0;
  sourceId: number = 0;
  sequence: number = 0;
  inputTime: number = 0;
  admittedH: number = 0;
  admittedL: number = 0;
  cumulativeCount: number | null = null;
}

export class P011ExternalAdmissionEvidence {
  revision: string = 'p011-external-admissions-v1';
  count: number | null = 0;
  admittedH: number | null = 0;
  admittedL: number | null = 0;
  observedTransactions: number = 0;
  lastRecord: P011ExternalAdmissionRecord | null = null;
  // A checkpoint/summary is not a complete raw admission history.
  rawHistoryStatus: string = 'SUMMARY_ONLY';
  exportFailures: number = 0;
}

export class P011ExternalSourceSnapshot {
  revision: string = 'p011-external-source-v1';
  initialWater: number = 0;
  initialHeavy: number = 0;
  initialLight: number = 0;
  sourceInjectedH: number = 0;
  sourceInjectedL: number = 0;
  sourceEpoch: number = 0;
  admissions: P011ExternalAdmissionEvidence = new P011ExternalAdmissionEvidence();
}

function copyRecord(value: P011ExternalAdmissionRecord): P011ExternalAdmissionRecord {
  const out = new P011ExternalAdmissionRecord();
  out.runId = value.runId; out.admissionTransactionId = value.admissionTransactionId;
  out.sourceId = value.sourceId; out.sequence = value.sequence; out.inputTime = value.inputTime;
  out.admittedH = value.admittedH; out.admittedL = value.admittedL;
  out.cumulativeCount = value.cumulativeCount;
  return out;
}

export class P011ExternalAdmissions {
  private evidence: P011ExternalAdmissionEvidence = new P011ExternalAdmissionEvidence();
  private sink: ((record: P011ExternalAdmissionRecord) => void) | null = null;

  get count(): number | null { return this.evidence.count; }

  reset(): void {
    this.evidence = new P011ExternalAdmissionEvidence();
    if (this.sink !== null) this.evidence.rawHistoryStatus = 'STREAMED';
  }

  invalidateHistory(): void {
    this.evidence = new P011ExternalAdmissionEvidence();
    this.evidence.count = null;
    this.evidence.admittedH = null; this.evidence.admittedL = null;
    this.evidence.rawHistoryStatus = 'UNKNOWN';
  }

  setSink(sink: ((record: P011ExternalAdmissionRecord) => void) | null): void {
    if (this.sink !== sink) {
      this.evidence.rawHistoryStatus = sink !== null && this.evidence.count === 0
        ? 'STREAMED' : this.evidence.count === null ? 'UNKNOWN' : 'SUMMARY_ONLY';
    }
    this.sink = sink;
  }

  // Called before changing any source counters. The prepared record is the
  // committed evidence; rejected attempts never reach this method.
  prepare(runId: string, sourceId: number, sequence: number, inputTime: number,
    admittedH: number, admittedL: number): P011ExternalAdmissionRecord {
    const e = this.evidence;
    if (!Number.isSafeInteger(e.observedTransactions + 1) ||
      (e.count !== null && !Number.isSafeInteger(e.count + 1)) ||
      !Number.isFinite(admittedH) || !Number.isFinite(admittedL) ||
      admittedH < 0 || admittedL < 0 || admittedH + admittedL <= 0 ||
      (e.admittedH !== null && !Number.isFinite(e.admittedH + admittedH)) ||
      (e.admittedL !== null && !Number.isFinite(e.admittedL + admittedL))) {
      throw new Error('INVALID_EXTERNAL_ADMISSION_RECORD');
    }
    const record = new P011ExternalAdmissionRecord();
    record.runId = runId; record.admissionTransactionId = e.observedTransactions + 1;
    record.sourceId = sourceId; record.sequence = sequence; record.inputTime = inputTime;
    record.admittedH = admittedH; record.admittedL = admittedL;
    record.cumulativeCount = e.count === null ? null : e.count + 1;
    return record;
  }

  // Same synchronous transaction as IH/IL/epoch publish; no callbacks here.
  commit(record: P011ExternalAdmissionRecord): void {
    const e = this.evidence;
    if (record.admissionTransactionId !== e.observedTransactions + 1 ||
      record.cumulativeCount !== (e.count === null ? null : e.count + 1)) {
      throw new Error('DUPLICATE_EXTERNAL_ADMISSION_RECORD');
    }
    e.observedTransactions = record.admissionTransactionId;
    e.count = record.cumulativeCount;
    if (e.admittedH !== null) e.admittedH += record.admittedH;
    if (e.admittedL !== null) e.admittedL += record.admittedL;
    e.lastRecord = record;
  }

  // Export is a copy of already committed evidence. Callback loss/replay does
  // not recount a transaction, nor can a consumer mutate the canonical record.
  exportLast(): void {
    const last = this.evidence.lastRecord;
    if (this.sink !== null && last !== null) {
      try { this.sink(copyRecord(last)); } catch (error) {
        this.evidence.exportFailures++;
        this.evidence.rawHistoryStatus = 'INCOMPLETE';
      }
    }
  }

  copyEvidenceTo(out: P011ExternalAdmissionEvidence): void {
    const e = this.evidence;
    out.revision = e.revision; out.count = e.count;
    out.admittedH = e.admittedH; out.admittedL = e.admittedL;
    out.observedTransactions = e.observedTransactions;
    out.rawHistoryStatus = e.rawHistoryStatus; out.exportFailures = e.exportFailures;
    if (e.lastRecord === null) out.lastRecord = null;
    else {
      if (out.lastRecord === null) out.lastRecord = new P011ExternalAdmissionRecord();
      const last = out.lastRecord;
      last.runId = e.lastRecord.runId;
      last.admissionTransactionId = e.lastRecord.admissionTransactionId;
      last.sourceId = e.lastRecord.sourceId; last.sequence = e.lastRecord.sequence;
      last.inputTime = e.lastRecord.inputTime;
      last.admittedH = e.lastRecord.admittedH; last.admittedL = e.lastRecord.admittedL;
      last.cumulativeCount = e.lastRecord.cumulativeCount;
    }
  }

  snapshot(): P011ExternalAdmissionEvidence {
    const out = new P011ExternalAdmissionEvidence(); this.copyEvidenceTo(out); return out;
  }

  // Validate before the caller changes any field/pool state. Legacy absence
  // is handled separately, never by copying epoch into the missing count.
  validateSource(value: P011ExternalSourceSnapshot): void {
    const e = value.admissions;
    if (value.revision !== 'p011-external-source-v1' || e === undefined ||
      e.revision !== 'p011-external-admissions-v1' ||
      !Number.isSafeInteger(value.sourceEpoch) || value.sourceEpoch < 0 ||
      !Number.isFinite(value.initialWater) || value.initialWater < 0 ||
      !Number.isFinite(value.initialHeavy) || value.initialHeavy < 0 ||
      !Number.isFinite(value.initialLight) || value.initialLight < 0 ||
      !Number.isFinite(value.sourceInjectedH) || value.sourceInjectedH < 0 ||
      !Number.isFinite(value.sourceInjectedL) || value.sourceInjectedL < 0 ||
      !Number.isSafeInteger(e.observedTransactions) || e.observedTransactions < 0 ||
      !Number.isSafeInteger(e.exportFailures) || e.exportFailures < 0) {
      throw new Error('INVALID_EXTERNAL_SOURCE_SNAPSHOT');
    }
    if (e.count !== null && (!Number.isSafeInteger(e.count) || e.count < 0 ||
      e.count !== value.sourceEpoch || e.count !== e.observedTransactions ||
      e.admittedH !== value.sourceInjectedH || e.admittedL !== value.sourceInjectedL)) {
      throw new Error('EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH');
    }
    if (e.count === null && (e.admittedH !== null || e.admittedL !== null)) {
      throw new Error('INVALID_EXTERNAL_SOURCE_SNAPSHOT');
    }
    if (e.observedTransactions === 0 && e.lastRecord !== null) {
      throw new Error('INVALID_EXTERNAL_SOURCE_SNAPSHOT');
    }
    if (e.observedTransactions > 0) {
      const r = e.lastRecord;
      if (r === null || r.admissionTransactionId !== e.observedTransactions ||
        r.cumulativeCount !== e.count || !Number.isFinite(r.admittedH) ||
        !Number.isFinite(r.admittedL) || r.admittedH < 0 || r.admittedL < 0 ||
        r.admittedH + r.admittedL <= 0) throw new Error('INVALID_EXTERNAL_SOURCE_SNAPSHOT');
    }
  }

  restore(value: P011ExternalAdmissionEvidence): void {
    // Copy into owned storage; the checkpoint does not restore a raw stream.
    const out = new P011ExternalAdmissionEvidence();
    out.count = value.count; out.admittedH = value.admittedH; out.admittedL = value.admittedL;
    out.observedTransactions = value.observedTransactions;
    out.lastRecord = value.lastRecord === null ? null : copyRecord(value.lastRecord);
    out.exportFailures = value.exportFailures;
    out.rawHistoryStatus = value.count === null ? 'UNKNOWN' : 'SUMMARY_ONLY';
    this.evidence = out;
  }
}
