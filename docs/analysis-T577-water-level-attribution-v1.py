"""T577 architect: source audit and fixed-expression algebra, no product test.
Does not import/run/modify the T568 oracle or any designer check program.
Does not implement a fluid stepper or a replacement oracle.
"""
from pathlib import Path
import ast
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]
D = ROOT / 'docs'
Q = 1 << 20
CQ = 256 * Q


def receipt(path):
    raw = (ROOT / path).read_bytes()
    return dict(path=path, bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())


def emit(name, obj):
    (D / name).write_text(json.dumps(obj, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')


path = 'test/t568/independent_oracle.py'
source = (ROOT / path).read_text(encoding='utf-8-sig')
tree = ast.parse(source)
funcs = {n.name: n for n in tree.body if isinstance(n, ast.FunctionDef)}
required = ['closed_form_noP', 'c02', 'c03', 'c05', 'eta4']
assert all(n in funcs for n in required)
fragments = {n: dict(line=funcs[n].lineno, endLine=funcs[n].end_lineno,
                     source=ast.get_source_segment(source, funcs[n])) for n in required}
closed_calls = [n for n in ast.walk(funcs['c02']) if isinstance(n, ast.Call)
                and isinstance(n.func, ast.Name) and n.func.id=='closed_form_noP']
assert len(closed_calls)==1 and ast.unparse(closed_calls[0].args[1])=='B'
assert 'Bq = B * Q' in fragments['c02']['source']
assert not any(isinstance(n, ast.Assert) for n in ast.walk(funcs['c02']))
assert 'closed_match' in fragments['c02']['source']
assert 'H = 80 - bed' in fragments['c03']['source']
assert 'qW = 0' in fragments['c03']['source']

# Direct substitution into the stated piecewise formula on eight supplied points.
# B is ALWAYS Q20; H4/eta4/R4/W4 are the 4Q lattice integers.
B = 44*Q
cap_h4 = 4*CQ-2*B
points = [0, 1, 2*B-1, 2*B, 2*B+1, 4*27*Q, cap_h4, cap_h4+1]
rows = []
for h4 in points:
    if h4>cap_h4:
        rows.append(dict(BQ20=B,H4=h4,status='CAPACITY_REJECT_BEFORE_SEARCH'))
        continue
    if h4<=2*B:
        eta, r, w = 2*h4, h4, 0
        branch = 'PORE_ONLY'
    else:
        eta, r, w = h4+2*B, 2*B, h4-2*B
        branch = 'FREE_WATER'
    actual_c8 = 2*eta-min(eta,4*B)
    assert actual_c8==2*h4 and h4==r+w and eta<=4*CQ
    rows.append(dict(BQ20=B,H4=h4,branch=branch,eta4=eta,R4=r,W4=w,
                     C8=actual_c8,target=2*h4,residual=actual_c8-2*h4))

H4 = 4*27*Q
correct_eta4 = H4+2*B
wrong_first_branch_eta4 = 2*H4
wrong_first_branch_c8 = 2*wrong_first_branch_eta4-4*B
assert correct_eta4==196*Q and wrong_first_branch_eta4==216*Q
assert wrong_first_branch_c8==256*Q and 2*H4==216*Q

# Literal behavior of the existing c02 call: B=44, H4=108Q picks ELSE,
# so it is NOT the 54-height first branch described as the observed fork.
wrong_call_eta4 = H4+2*44
wrong_call_r4 = 2*44
wrong_call_w4 = H4-2*44
assert wrong_call_eta4<4*B
wrong_call_c8 = wrong_call_eta4  # below 4B: pore-only capacity slope 1
assert wrong_call_c8!=2*H4

# Closed interval [0, 2^30] contains 2^30+1 candidates. The maximum width
# becomes floor(width/2) each lower_bound step: 31 steps suffice, not 30.
width = 4*CQ
widths = [width]
while width:
    width //= 2
    widths.append(width)
assert len(widths)-1==31 and widths[-2:]==[1,0]

calm = []
for b in [20,40,60]:
    bq = b*Q
    actual_h4 = (80-b)*4*Q  # the source literal, not the correct construction
    source_eta4 = 2*actual_h4 if actual_h4<=2*bq else actual_h4+2*bq
    required_h4 = 4*80*Q-2*bq
    assert required_h4==4*Q*(80-b//2)
    calm.append(dict(bed=b,sourceH4=actual_h4,sourceEta4=source_eta4,
                     sourceEtaHeight=source_eta4//(4*Q),requiredH4=required_h4,
                     requiredWaterVolume=required_h4//(4*Q),requiredEta4=4*80*Q))
assert [r['sourceEtaHeight'] for r in calm]==[70,60,40]

contract = {
    'schema':'T577_G12_GEOMETRY_CLARIFICATION_V1', 'date':'2026-11-18',
    'classification':'TEST_IMPLEMENTATION_UNITS_AND_ASSERTIONS_REPORT_ATTRIBUTION_ERROR',
    'designEquationChanged':False, 'T567BinaryContractChanged':False,
    'units':{'B':'S+F in Q20; b physical height=B/Q', 'H4':'4Q*physical total water volume',
             'eta4':'4Q*physical water height', 'R4':'4Q*pore water volume','W4':'4Q*free water volume'},
    'noP':{'domain':'no active P in this column, valid B/H4/J reservation and all normal input gates',
            'capacity':'0<=B<=CQ; H4>=0; 2B+H4+J4<=4CQ',
            'C8':'z4 for0<=z4<=4B, otherwise2z4-4B',
            'branches':[
                {'if':'H4<=2B','eta4':'2H4','R4':'H4','W4':'0'},
                {'if':'H4>2B','eta4':'H4+2B','R4':'2B','W4':'H4-2B'}],
            'physicalUnits':'h<=b/2 -> eta=2h; h>b/2 -> eta=h+b/2',
            'generalValidity':'both branches satisfy C8(eta4)=2H4 over their complete valid no-P domain',
            'productionPolicy':'T576 keeps existing lower_bound+exact-equality contract; no new fast path is authorized by this clarification'},
    'search':{'interval':[0,4*CQ],'maxRounds':31,'widthSequence':widths,
              'preconditions':['nonnegative valid inventories and dual capacity',
                               'bed/P endpoints even and within[0,4CQ]',
                               'same-column P slabs disjoint and outside bed; equality at endpoints allowed'],
              'proof':'C8 has slope1 inside one solid slab and2 outside; even slab endpoints make all even targets attainable; exact equality checked, not approximate convergence',
              'H4Zero':'eta4=0 and no water surface',
              'reject':'bad geometry/capacity is rejected before search; never clamp H/eta or union overlap to manufacture an equality'},
    'T576':{'newDesignDependency':False,'newArchitectApprovalGate':False,
             'realPrerequisite':'T573 complete DM parent backend remains unchanged',
             'implementation':'existing C8 lower_bound plus exact equality remains valid; preserve input units/normal gates',
             'noABIResourceDispatchChange':True,'GPUStatus':'NOT_IMPLEMENTED_NOT_RUN evidence retained'},
    'routing':{'tester':'repair existing T568 static oracle units, independent assertions and report; preserve v1 artifacts, deliver new version',
               'designer':'no model/closed-form correction required by the supplied counterexample',
               'developer':'T576 consumes this unit clarification with original binary C8 contract; not a repair for an observed GPU bug',
               'manager':'register test implementation/report correction and follow-up; no threshold dispute or new acceptance gate'}
}
emit('architecture-T577-geometry-clarification-v1.json',contract)
emit('analysis-T577-water-level-attribution-v1.json',{
    'schema':'T577_SOURCE_AND_FIXED_ALGEBRA_AUDIT_V1','date':'2026-11-18',
    'result':'ATTRIBUTION_COMPLETE_NOT_PRODUCT_PASS','deviceExecuted':False,
    'oldOracleExecuted':False,'productTestsRerun':False,'CPUPhysicsAdded':False,
    'inputs':[receipt(p) for p in [path,'docs/test-report-T568-g12-oracle-v1.md',
        'docs/algorithm-G12-water-sfp-coupling-v1.md','docs/design-G12-water-sfp-fixtures-v1.json',
        'docs/architecture-T567-water-sfp-engineering-v1.md','docs/architecture-T567-g12-engineering-v1.json']],
    'originalSourceFragments':fragments,
    'sourceFindings':{'C02ClosedArg':'B=44 physical, should Bq=44Q for the stated helper formula',
        'helperDocstring':'says B/H4 both Q22; B must be Q20 for this exact formula',
        'closedAssertionMissing':'closed_match booleans returned but neither asserted nor included in fixture match predicate',
        'C03':'H=80-bed yields eta70/60/40; qW assigned literal0 so all_qW_zero does not test equal-head flux',
        'C05':'2B+Hphysical<=4*256 mixes scales; true joint scalar in physical units is B/2+H<=256 (P=J=0)'},
    'C02LegalNoPFixedPoints':rows,
    'reportPreservedCounterexample':{'S':40,'F':4,'H':27,'correctEtaHeight':49,
        'correct':{'BQ20':B,'H4':H4,'eta4':correct_eta4,'R4':2*B,'W4':H4-2*B},
        'wrongLowBranchOutsideDomain':{'eta4':wrong_first_branch_eta4,'etaHeight':54,
            'C8':wrong_first_branch_c8,'target':2*H4,'residual':wrong_first_branch_c8-2*H4},
        'actualWrongUnitCall':{'BArgument':44,'branch':'ELSE','eta4':wrong_call_eta4,
            'R4':wrong_call_r4,'W4':wrong_call_w4,'C8':wrong_call_c8,'target':2*H4}},
    'C03ConstructionAudit':calm,
    'C05SuppliedNearCapacityCounterexample':{'B':44,'H':235,
        'wrongScalar':2*44+235,'wrongLimit':4*256,'wrongExpressionAccepts':2*44+235<=4*256,
        'trueOccupancy':44//2+235,'trueLimit':256,'trueCapacityAccepts':44//2+235<=256},
    'maxSearchRounds':len(widths)-1,
    'notEstablished':['T568 eight scenarios fully validated','wet GPU time evolution','all GPU/cold/scene correctness',
                      'energy stability/standard wet-case progress','real device deadline/performance']
})
print(json.dumps({'T577':'source attribution and fixed-expression audit complete',
                  'etaHeight':49,'maxSearchRounds':31,'designChangeRequired':False,
                  'T576NewDesignBlocker':False,'oldOracleExecuted':False},ensure_ascii=False))
