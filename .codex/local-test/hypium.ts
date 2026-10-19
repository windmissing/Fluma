const selectedTests: Array<{ name: string; run: () => void }> = [];

function describe(name: string, suite: () => void): void {
  suite();
}

function it(name: string, _id: number, run: () => void): void {
  if (/^p011_(multiChannelTotalBudgetScalesAllRequestsProportionally|fullLoadKeepsBottomCreepIndependentBudget|totalBudgetExhaustionScalesRequestsProportionally)$/.test(name)) {
    selectedTests.push({ name, run });
  }
}

function expect(actual: unknown) {
  const location = new Error().stack ?? '';
  return {
    assertTrue(): void {
      if (actual !== true) throw new Error(`expected true, actual ${String(actual)}\n${location}`);
    },
    assertFalse(): void {
      if (actual !== false) throw new Error(`expected false, actual ${String(actual)}\n${location}`);
    },
    assertEqual(expected: unknown): void {
      if (actual !== expected) throw new Error(`expected ${String(expected)}, actual ${String(actual)}\n${location}`);
    },
    assertContain(expected: unknown): void {
      if (!String(actual).includes(String(expected))) throw new Error(`expected ${String(actual)} to contain ${String(expected)}\n${location}`);
    }
  };
}

export { selectedTests, describe, it, expect };
