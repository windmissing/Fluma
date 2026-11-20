"""T716: RX readonly storage reads at uint value-parameter boundaries."""
from pathlib import Path
import sys

QUALIFIED_VALUES = Path(__file__).resolve().parents[1] / 'glsl' / 'qualified_values.py'
sys.path.insert(0, str(QUALIFIED_VALUES.parent))
from qualified_values import copy_value_arguments

REVISION = 'RX_READONLY_VALUE_V1'


def convert_readonly_values(source):
    declaration = 'readonly buffer ReadState{uint B[];}'
    if source.count(declaration) != 1:
        raise ValueError('RX_READONLY_STORAGE_DECLARATION')
    return copy_value_arguments(source, {'B': 'rxReadBaseWord'},
                                {('g9EventMask', 0, 'B'): 1},
                                'uvec4 g9ReadWide(uint at)')
