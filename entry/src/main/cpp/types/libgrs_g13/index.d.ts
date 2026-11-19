export const g13Configure: (config128: Uint32Array, seedReceiptJSON: string) => string;
export const g13Offer128: (input: Uint32Array) => string;
export const g13PagePulse: (session: string, sequence: string, uiNs: string) => string;
export const g13Stop: (scopeJSON: string) => string;
export const g13Status: () => string;
export const g13ColdBegin: (token: string, mode: string) => string;
export const g13ColdPage: (id: string, segment: number, wordOffset: number) => string;
export const g13ColdEnd: (id: string) => string;
export const g13DiagnosticBegin: (config128: Uint32Array, fixtureReceiptJSON: string) => string;
export const g13BeginSession: (scopeJSON: string) => string;
export const g13SetActive: (active: boolean, scopeJSON: string) => string;
export const g13ViewMode: (mode: string, scales64: Uint32Array) => string;

export const g13InitDiagnostics: (attemptId: string, page: number) => string;
