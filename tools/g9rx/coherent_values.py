"""T709: classify RX coherent scalar arguments against actual GLSL signatures."""
from collections import Counter
import re

REVISION = 'RX_COHERENT_VALUE_V2'
READERS = {'C': 'rxReadCandidateWord', 'W': 'rxReadWorkWord', 'E': 'rxReadEventWord'}
# Every discovered user-function boundary has to match this reviewed set.
EXPECTED = {
    ('g9MapAdd', 3, 'C'): 1,
    ('g9U32', 0, 'C'): 17,
    ('g9U32', 0, 'W'): 18,
    ('g9ColumnPotential', 1, 'W'): 1,
    ('g9ColumnPotential', 2, 'W'): 1,
    ('dmPut', 2, 'C'): 2,
    ('dmPut', 2, 'W'): 1,
    ('g9ProvisionalAdd', 2, 'W'): 1,
    ('g9PutColumn', 2, 'W'): 1,
}


def argument_spans(text, begin):
    start = begin
    depth = 0
    for offset in range(begin, len(text)):
        ch = text[offset]
        if depth == 0 and ch in ',)':
            yield start, offset
            if ch == ')':
                return
            start = offset + 1
        elif ch in '([':
            depth += 1
        elif ch in ')]':
            depth -= 1
    raise ValueError('RX_COHERENT_UNCLOSED_CALL')


def direct_reads(source):
    # Preserve line and byte offsets while excluding comments from GLSL syntax.
    text = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/',
                  lambda m: re.sub(r'[^\n]', ' ', m.group()), source)
    signatures = {}
    for match in re.finditer(r'\b(?:void|bool|u?int|float|[biu]?vec[234]|G9\w+)\s+'
                             r'(\w+)\s*\(([^()]*)\)\s*\{', text):
        name = match[1]
        params = match[2].split(',') if match[2].strip() else []
        signatures.setdefault((name, len(params)), []).append(params)
    for match in re.finditer(r'\b([A-Za-z_]\w*)\s*\(', text):
        name = match[1]
        spans = list(argument_spans(text, match.end()))
        signatures_at = signatures.get((name, len(spans)), [])
        for index, (start, end) in enumerate(spans):
            arg = text[start:end].strip()
            read = re.fullmatch(r'([CWE])\s*\[(.+)\]', arg, re.S)
            if not read or not complete_index(read[2]):
                continue
            start += len(text[start:end]) - len(text[start:end].lstrip())
            end = start + len(arg)
            if signatures_at:
                for params in signatures_at:
                    if not re.fullmatch(r'\s*(?:in\s+)?uint\s+\w+\s*', params[index]):
                        raise ValueError(f'RX_COHERENT_NOT_UINT_VALUE {name}:{index}')
            yield name, index, read[1], read[2], start, end, bool(signatures_at)


def complete_index(text):
    depth = 0
    for ch in text:
        if ch == '[':
            depth += 1
        elif ch == ']':
            depth -= 1
            if depth < 0:
                return False
    return depth == 0


def convert_values(source):
    reads = list(direct_reads(source))
    values = Counter((name, index, buf) for name, index, buf, _, _, _, user in reads if user)
    if values != EXPECTED:
        raise ValueError('RX_COHERENT_USER_BOUNDARIES_CHANGED ' + repr(dict(values)))
    edits = []
    for name, index, buf, expression, start, end, user in reads:
        if user:
            edits.append((start, end, {
                'callee': name, 'argument': index + 1, 'buffer': buf,
                'line': source.count('\n', 0, start) + 1,
                'before': source[start:end], 'after': f'{READERS[buf]}({expression})',
            }))
    revised = source
    for start, end, edit in sorted(edits, reverse=True):
        revised = revised[:start] + edit['after'] + revised[end:]
    # Place definitions at the start of the existing RX storage accessor block.
    anchor = 'uvec4 g9ReadWide(uint at)'
    if revised.count(anchor) != 1:
        raise ValueError('RX_COHERENT_READER_ANCHOR')
    helpers = ''.join(f'uint {READERS[buf]}(uint at){{return {buf}[at];}}\n'
                      for buf in READERS if any(key[2] == buf for key in values))
    helper_line = source.count('\n', 0, source.index(anchor)) + 1
    for _, _, edit in edits:
        edit['canonicalLine'] = edit['line'] + (helpers.count('\n') if edit['line'] >= helper_line else 0)
    revised = revised.replace(anchor, helpers + anchor)
    if any(user for *_, user in direct_reads(revised)):
        raise ValueError('RX_COHERENT_USER_ARGUMENT_REMAINS')
    return revised, [edit for _, _, edit in edits]
