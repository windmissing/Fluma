"""T425 mathematical examples and certificate counterexamples.

No product modules, shaders, device commands or existing test runners are run.
Python int/Fraction form an independent arithmetic reference for limb examples.
"""
from fractions import Fraction
from pathlib import Path
import hashlib
import json
import math
import random
import struct

ROOT = Path(__file__).resolve().parents[1]
LIMBS = 70
BASE = 1 << 16
LIMIT = 1 << (16 * LIMBS)
UNIT_DENOMINATOR = 1 << 1074


def float_bits(value):
    return struct.unpack('<Q', struct.pack('<d', value))[0]


def from_bits(bits):
    return struct.unpack('<d', struct.pack('<Q', bits))[0]


def encode_bits(bits):
    exponent = (bits >> 52) & 2047
    fraction = bits & ((1 << 52)-1)
    sign = bits >> 63
    if exponent == 2047 or (sign and (exponent or fraction)):
        raise ValueError('INPUT_INVALID')
    if exponent > 1031 or (exponent == 1031 and fraction):
        raise ValueError('INPUT_INVALID')
    return fraction if exponent == 0 else ((1 << 52) | fraction) << (exponent-1)


def rounded_bits(integer):
    assert 0 <= integer < LIMIT
    length = integer.bit_length()
    if length <= 52:
        return integer
    shift = max(0, length-53)
    quotient = integer >> shift
    if shift:
        remainder = integer - (quotient << shift)
        half = 1 << (shift-1)
        if remainder > half or (remainder == half and quotient & 1):
            quotient += 1
    if quotient == 1 << 53:
        quotient >>= 1
        shift += 1
    return ((shift+1) << 52) | (quotient-(1 << 52))


def limb_array(integer):
    assert 0 <= integer < LIMIT
    return [(integer >> (16*i)) & 65535 for i in range(LIMBS)]


def limb_integer(limbs):
    return sum(v << (16*i) for i, v in enumerate(limbs))


def add_atomic(limbs, integer):
    digits = limb_array(integer)
    carry = 0
    for a, b in zip(limbs, digits):
        carry = (a+b+carry) // BASE
    if carry:
        return 'ACC_OVERFLOW'
    carry = 0
    for i, (a, b) in enumerate(zip(limbs[:], digits)):
        value = a+b+carry
        limbs[i], carry = value % BASE, value // BASE
    return 'OK'


def subtract_atomic(limbs, integer):
    digits = limb_array(integer)
    for i in range(LIMBS-1, -1, -1):
        if limbs[i] != digits[i]:
            if limbs[i] < digits[i]:
                return 'ACC_NEGATIVE'
            break
    borrow = 0
    for i in range(LIMBS):
        value = limbs[i]-digits[i]-borrow
        borrow = int(value < 0)
        limbs[i] = value+BASE if borrow else value
    return 'OK'


def ordered_fold(values):
    value = 0.0
    for number in values:
        value = value+number
    return value


def fold_prefix(values):
    prefixes = []
    running = 0.0
    for number in values:
        running = running+number
        prefixes.append(running)
    return prefixes


class ExampleWordTransaction:
    """Small word-log example for branch and shadow rollback reasoning."""

    def __init__(self):
        # Logical mass, ID, business slot generation, shadow mass and dirty flag.
        self.raw = bytearray(struct.pack('<5Q', 1, 81, 7, 1, 0))
        self.log = []
        self.marks = []
        self.proof_epoch = 1

    def mark(self):
        mark = (len(self.log), len(self.marks))
        self.marks.append(mark)
        return mark

    def write(self, word, value):
        offset = word*8
        self.log.append((offset, bytes(self.raw[offset:offset+8])))
        self.raw[offset:offset+8] = struct.pack('<Q', value)

    def commit_child(self, mark):
        assert self.marks[-1] == mark and len(self.marks) > 1
        self.marks.pop()  # Entries are retained for the enclosing mark.

    def commit_root(self, mark):
        assert self.marks == [mark]
        self.marks.clear()
        self.log.clear()
        self.proof_epoch += 1

    def rollback(self, mark):
        assert mark in self.marks
        while len(self.log) > mark[0]:
            offset, before = self.log.pop()
            self.raw[offset:offset+8] = before
        del self.marks[mark[1]:]
        self.proof_epoch += 1

    def words(self):
        return struct.unpack('<5Q', self.raw)

    def key(self):
        words = self.words()
        return words[1], words[2], self.proof_epoch


def main():
    rng = random.Random(0x425)
    evidence = {'scope': 'DESIGN_MATH_AND_COUNTEREXAMPLES',
                'date': '2026-11-09', 'product_execution': False,
                'device_execution': False, 'cases': {}, 'measurements': {}}
    cases, measurements = evidence['cases'], evidence['measurements']
    assert encode_bits(float_bits(0.0)) == encode_bits(float_bits(-0.0)) == 0
    assert float_bits(0.0) != float_bits(-0.0)
    invalid = [float_bits(-1.), float_bits(float('inf')), 0x7ff8000000000042,
               float_bits(math.nextafter(256., math.inf))]
    for bits in invalid:
        try:
            encode_bits(bits)
        except ValueError:
            pass
        else:
            raise AssertionError(bits)
    slots = [None]*96
    acc = {}
    counts = {}
    exact_operations = 0
    for iteration in range(3000):
        slot = rng.randrange(len(slots))
        old = slots[slot]
        if old is not None:
            bucket, mass = old
            assert subtract_atomic(acc[bucket], encode_bits(float_bits(mass))) == 'OK'
            counts[bucket] -= 1
            exact_operations += 1
        if rng.randrange(5) == 0:
            slots[slot] = None
        else:
            bucket = rng.randrange(256)
            exp = rng.randrange(1032)
            fraction = 0 if exp == 1031 else rng.getrandbits(52)
            bits = (exp << 52) | fraction
            if bits == 0:
                bits = 1
            mass = from_bits(bits)
            acc.setdefault(bucket, [0]*70)
            assert add_atomic(acc[bucket], encode_bits(bits)) == 'OK'
            counts[bucket] = counts.get(bucket, 0)+1
            slots[slot] = (bucket, mass)
            exact_operations += 1
        # Rebuild the independent reference from actual post-cut membership.
        expected, expected_counts = {}, {}
        for member in slots:
            if member is not None:
                bucket, mass = member
                expected[bucket] = expected.get(bucket, 0)+encode_bits(float_bits(mass))
                expected_counts[bucket] = expected_counts.get(bucket, 0)+1
        for bucket in acc:
            units = expected.get(bucket, 0)
            assert limb_integer(acc[bucket]) == units
            assert counts[bucket] == expected_counts.get(bucket, 0)
            # Fraction conversion performs an independent binary64 rounding.
            assert rounded_bits(units) == float_bits(float(Fraction(units, UNIT_DENOMINATOR)))
    cases['E01_exact_membership_mutation_cuts'] = 3000
    measurements['E01_checked_limb_operations'] = exact_operations
    edge_units = [0, 1, 2, (1 << 52)-1, 1 << 52, (1 << 53)-1,
                  (1 << 53)+1, (1 << 53)+3,
                  (1 << 1074)+(1 << 1021),
                  (1 << 1074)+(3 << 1021),
                  (1 << 1074)+(1 << 1021)+1,
                  (1 << 1075)-(1 << 1021), LIMIT-1]
    for units in edge_units:
        assert rounded_bits(units) == float_bits(float(Fraction(units, UNIT_DENOMINATOR)))
    cases['E02_rounding_boundary_cases'] = len(edge_units)
    # Bound is a sum of legal per-slot values, not one legal individual mass.
    max_units = 512*encode_bits(float_bits(256.))
    assert max_units.bit_length() == 1092 and max_units < LIMIT
    measurements['E02_valid_512_slot_max_sum_bit_length'] = max_units.bit_length()
    low = limb_array(1)
    assert subtract_atomic(low, 2) == 'ACC_NEGATIVE' and limb_integer(low) == 1
    high = limb_array(LIMIT-1)
    assert add_atomic(high, 1) == 'ACC_OVERFLOW' and limb_integer(high) == LIMIT-1
    assert 1-2+2 == 1 and LIMIT-1+1-1 == LIMIT-1
    cases['E03_checked_prefix_netting_counterexamples'] = 2
    tiny = 2.**-53
    values = [1., tiny, tiny]
    serial = ordered_fold(values)
    tree = 1.+(tiny+tiny)
    exact = float(Fraction(1)+Fraction.from_float(tiny)*2)
    assert serial == 1. and tree == exact == math.nextafter(1., math.inf)
    cases['E04_RN64_tree_and_exact_differ'] = True
    measurements['E04_bits'] = {'serial': hex(float_bits(serial)), 'tree': hex(float_bits(tree))}
    # Remove the first physical slot. Its discarded small terms reappear in a cold fold.
    delta_total = serial-1.
    fresh_total = ordered_fold([tiny, tiny])
    assert delta_total == 0 and fresh_total == 2.**-52
    cases['E05_total_minus_removed_term_counterexample'] = True
    max_suffix = 0
    for _ in range(1500):
        old_values = [from_bits((rng.randrange(1, 1031) << 52) | rng.getrandbits(52))
                      for i in range(32)]
        old_prefix = fold_prefix(old_values)
        first = rng.randrange(32)
        new_values = old_values[:]
        new_values[first] = from_bits((rng.randrange(1, 1031) << 52) | rng.getrandbits(52))
        running = old_prefix[first-1] if first else 0.
        work = 0
        for i in range(first, 32):
            running = running+new_values[i]
            work += 1
            if float_bits(running) == float_bits(old_prefix[i]):
                running = old_prefix[-1]
                break
        assert float_bits(running) == float_bits(ordered_fold(new_values))
        max_suffix = max(max_suffix, work)
    worst_old = [1.]+[tiny]*31
    worst_new = [2.]+[tiny]*31
    assert all(a != b for a, b in zip(fold_prefix(worst_old), fold_prefix(worst_new)))
    cases['E06_ordered_suffix_random_cases'] = 1500
    measurements['E06_forced_prefix_replay_items'] = len(worst_new)
    # Matching one prefix cannot skip a changed later member.
    assert float_bits(ordered_fold([1., 2., 3.])) != float_bits(ordered_fold([1., 2., 4.]))
    cases['E06_later_dirty_stops_early_exit'] = True
    raw = bytearray(struct.pack('<QQQ', 0x8000000000000000, 1, 0x7ff8000000000042))
    saved = bytes(raw)
    logs = []
    for offset, word in [(0, 0), (8, 0), (16, 0), (0, float_bits(7.))]:
        logs.append((offset, bytes(raw[offset:offset+8])))
        raw[offset:offset+8] = struct.pack('<Q', word)
    for offset, before in reversed(logs):
        raw[offset:offset+8] = before
    assert bytes(raw) == saved
    assert from_bits(1) != 0 and encode_bits(1) == 1
    assert from_bits(0x8000000000000000) == from_bits(0)
    cases['E07_raw_signed_zero_subnormal_nan_undo'] = True
    serial = 5
    good = bytearray(struct.pack('<d', 1.))
    same_alias = memoryview(good)
    same_alias[:] = struct.pack('<d', 2.)
    assert serial == 5 and struct.unpack('<d', good)[0] != 1.
    wrong_limb = limb_array(encode_bits(float_bits(1.)))
    wrong_limb[0] ^= 1
    assert limb_integer(wrong_limb) != encode_bits(float_bits(1.))
    cases['E08_alias_same_serial_and_canonical_only_corruption'] = 2
    # The verifier stages are the original public-boundary order.
    order = ['POOL_BINDING', 'PUBLIC_ALLOCATOR_ID', 'PUBLIC_POOL', 'ACC_MISMATCH',
             'COUNT_MISMATCH', 'CACHE_BITS_MISMATCH', 'PUBLIC_STRICT_DOMAIN']
    eager_mismatches = 0
    for a in range(len(order)):
        for b in range(len(order)):
            bad = {order[a], order[b]}
            original = next(name for name in order if name in bad)
            proof_suspicion = order[b]
            # A suspect proof invokes the original sequence at this cut.
            fallback = next(name for name in order if name in bad)
            assert fallback == original
            eager_mismatches += int(proof_suspicion != original)
    assert order.index('PUBLIC_ALLOCATOR_ID') < order.index('ACC_MISMATCH')
    cases['E09_first_stage_pair_cases'] = len(order)**2
    measurements['E09_eager_fast_fail_wrong_stage_pairs'] = eager_mismatches
    tx = ExampleWordTransaction()
    original_bytes, old_key = bytes(tx.raw), tx.key()
    parent = tx.mark()
    tx.write(0, 2)
    tx.write(3, 2)
    tx.write(4, 1)
    child = tx.mark()
    for word, value in [(0, 0), (1, 0), (2, 8), (3, 0),
                        (0, 3), (1, 82), (2, 9), (3, 3)]:
        tx.write(word, value)
    frozen_before_rollback = bytes(tx.raw)
    tx.commit_child(child)
    assert tx.words() == (3, 82, 9, 3, 1)
    tx.rollback(parent)
    assert bytes(tx.raw) == original_bytes and tx.key() != old_key
    assert frozen_before_rollback != bytes(tx.raw)
    # A published source and first step survive a later failed step.
    source = tx.mark()
    tx.write(0, 5)
    tx.write(3, 5)
    tx.commit_root(source)
    first_step = tx.mark()
    tx.write(0, 6)
    tx.write(3, 6)
    tx.commit_root(first_step)
    successful_prefix = bytes(tx.raw)
    second_step = tx.mark()
    tx.write(0, 9)
    tx.write(3, 9)
    tx.rollback(second_step)
    assert bytes(tx.raw) == successful_prefix and tx.words()[0] == 6
    cases['E10_same_slot_branch_ABA_rejected'] = True
    cases['E10_child_commit_parent_rollback_and_success_prefix'] = True
    deps = {
        'mass': {'exact', 'ordered_fold', 'strict', 'bytes'},
        'vy': {'structure', 'bytes'},
        'bh': {'strict', 'column_fold', 'bytes'},
        'capacity': {'strict', 'all_columns'},
        'inactive_free_tail': {'bytes'},
        'canonical_limb': {'exact', 'bytes'},
    }
    assert 'strict' in deps['bh'] and 'exact' not in deps['vy']
    assert deps['inactive_free_tail'] == {'bytes'}
    free_stack = [1, 2]
    swapped_stack = [2, 1]
    assert set(free_stack) == set(swapped_stack) and free_stack[-1] != swapped_stack[-1]
    assert len([11, 12]) == len([11, 11]) and len(set([11, 11])) != 2
    cases['E11_free_order_and_duplicate_ID_counterexamples'] = 2
    bh, surface, cache, water_top = 1., 1., 0., 1.
    def column_valid(bed):
        top = ((bed+0.)+0.)+0.
        envelope = ((top+0.)+0.)+cache
        return surface == top and water_top == envelope and bed <= top <= envelope
    assert column_valid(bh)
    unchanged_pool_revision = 41
    bh = 2.
    assert unchanged_pool_revision == 41 and not column_valid(bh)
    assert 0.0 == -0.0 and float_bits(0.) != float_bits(-0.)
    cases['E11_dirty_dependencies_and_predicate_domains'] = len(deps)
    full_checks, hits, fallbacks = 0, 0, 0
    slots = [None]*32
    proof_sum = 0
    for _ in range(1500):
        i = rng.randrange(32)
        old = slots[i]
        units = rng.randrange(1, 1 << 20)
        new = from_bits(units)  # positive subnormal, exact sum within range
        if old is not None:
            proof_sum -= encode_bits(float_bits(old))
        slots[i] = new
        proof_sum += units
        unknown = rng.randrange(9) == 0
        if unknown:
            slots[(i+1) % 32] = from_bits(rng.randrange(1, 1 << 20))
            fallbacks += 1
            proof_sum = sum(encode_bits(float_bits(x)) for x in slots if x is not None)
        else:
            hits += 1
        cold = sum(encode_bits(float_bits(x)) for x in slots if x is not None)
        assert proof_sum == cold
        full_checks += 1
    cases['E12_model_sparse_cut_comparisons'] = full_checks
    measurements['E12_synthetic_proof_hits_fallbacks'] = [hits, fallbacks]
    # An old tolerated-float comparison can differ on an unproved damaged cache.
    # This input demonstrates the predicate boundary, not a valid V2 public state.
    serial = ordered_fold([1., tiny, tiny])
    exact = math.nextafter(1., math.inf)
    cache = math.nextafter(serial-1e-9, math.inf)
    assert abs(serial-cache) <= 1e-9 < abs(exact-cache)
    cases['E04_unproved_cache_tolerance_boundary_flip'] = True
    measurements['E04_tolerance_boundary'] = {'cache': cache,
        'serial_error': abs(serial-cache), 'exact_error': abs(exact-cache),
        'scope': 'damaged input predicate counterexample, not certified valid V2'}
    assert 39984+67*512 == 74288 and 39984+67*192 == 52848
    cases['resource_template_N512_N192'] = [74288, 52848]
    files = [
        'docs/architecture-T420-cpu-frame-complexity-v1.md',
        'docs/developer-report-T423-cpu-local-undo-v1.md',
        'docs/test-report-T424-cpu-baseline-matrix-v1.md',
        'entry/src/main/ets/physics/derived/P011Exact70.ets',
        'entry/src/main/ets/physics/derived/P011DerivedViewV2.ets',
        'entry/src/main/ets/physics/derived/P011DerivedTupleBarrierV2.ets',
        'entry/src/main/ets/physics/derived/P011DryDomainV2.ets',
        'entry/src/main/ets/physics/derived/P011DerivedCommitV2.ets',
        'entry/src/main/ets/physics/derived/P051DerivedFacadeV2.ets',
        'entry/src/main/ets/physics/derived/P011DerivedCheckpointV2.ets',
        'entry/src/main/ets/physics/P011ParticleHot.ets',
        'entry/src/main/ets/physics/tx/CpuLocalRowUndoV1.ets',
        'entry/src/main/ets/physics/tx/TxWordUndo.ets',
        'entry/src/main/ets/physics/tx/TxCertificates.ets',
        'docs/algorithm-T197-whole-transaction-complexity-v1.md',
        'docs/algorithm-T425-incremental-audit-equivalence-v1.md',
        'docs/retro-T425-incremental-audit-equivalence-v1.md',
        'docs/design-T425-audit-proof-model-v1.py',
    ]
    evidence['inputs'] = [{'path': name, 'bytes': (ROOT/name).stat().st_size,
                          'sha256': hashlib.sha256((ROOT/name).read_bytes()).hexdigest()}
                         for name in files]
    evidence['limitations'] = [
        'Proof is conditional on the P0-P8 writer/reader closure.',
        'No product audit was replaced or product regression run.',
        'Python model does not establish ArkTS device numerical conformance.',
        'No device timing, RSS, rendering or 30fps result.',
        'Actual work counters differ when scans are removed and require versioned accounting.',
    ]
    output = ROOT/'docs/design-T425-audit-proof-evidence-v1.json'
    output.write_text(json.dumps(evidence, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'cases': cases, 'measurements': measurements,
                      'input_fingerprints': len(files)}, indent=2))


if __name__ == '__main__':
    main()
