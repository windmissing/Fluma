"""Value-copy boundaries for generated GLSL storage-buffer reads."""
from collections import Counter
import re


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
    raise ValueError('QUALIFIED_VALUE_UNCLOSED_CALL')


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


def direct_value_arguments(source, buffers):
    # Mask comments without moving source offsets. Resolve declared user
    # functions and verify value uint parameters before changing actuals.
    text = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/',
                  lambda m: re.sub(r'[^\n]', ' ', m.group()), source)
    signatures = {}
    for match in re.finditer(r'\b(?:void|bool|u?int|float|[biu]?vec[234]|G9\w+)\s+'
                             r'(\w+)\s*\(([^()]*)\)\s*\{', text):
        name = match[1]
        params = match[2].split(',') if match[2].strip() else []
        signatures.setdefault((name, len(params)), []).append(params)
    names = {key[0] for key in signatures}
    selector = re.compile(r'([' + re.escape(''.join(buffers)) + r'])\s*\[(.+)\]', re.S)
    for match in re.finditer(r'\b([A-Za-z_]\w*)\s*\(', text):
        name = match[1]
        if name not in names:
            continue
        spans = list(argument_spans(text, match.end()))
        candidates = signatures.get((name, len(spans)), [])
        for index, (start, end) in enumerate(spans):
            arg = text[start:end].strip()
            read = selector.fullmatch(arg)
            if not read or not complete_index(read[2]):
                continue
            if not candidates or any(not re.fullmatch(r'\s*(?:in\s+)?uint\s+\w+\s*', params[index])
                                     for params in candidates):
                raise ValueError(f'QUALIFIED_VALUE_NOT_UINT_IN {name}:{index}')
            start += len(text[start:end]) - len(text[start:end].lstrip())
            left, right = read.span(2)
            yield name, index, read[1], source[start + left:start + right], start, start + len(arg)


def copy_value_arguments(source, readers, expected, anchor):
    arguments = list(direct_value_arguments(source, readers))
    observed = Counter((name, index, buffer) for name, index, buffer, *rest in arguments)
    if observed != expected:
        raise ValueError('QUALIFIED_VALUE_BOUNDARIES_CHANGED ' + repr(dict(observed)))
    if source.count(anchor) != 1:
        raise ValueError('QUALIFIED_VALUE_READER_ANCHOR')
    anchor_at = source.index(anchor)
    if any(key[4] < anchor_at for key in arguments):
        raise ValueError('QUALIFIED_VALUE_CALL_BEFORE_READER')
    used = {key[2] for key in observed}
    helpers = ''.join(f'uint {name}(uint at){{return {buf}[at];}}\n'
                      for buf, name in readers.items() if buf in used)
    if any(re.search(r'\b' + re.escape(readers[buf]) + r'\b', source) for buf in used):
        raise ValueError('QUALIFIED_VALUE_READER_NAME_COLLISION')
    pieces = []
    records = []
    prior = 0
    for name, index, buffer, expression, start, end in sorted(arguments, key=lambda row: row[4]):
        if start < prior:
            raise ValueError('QUALIFIED_VALUE_OVERLAPPING_ARGUMENTS')
        replacement = f'{readers[buffer]}({expression})'
        line = source.count('\n', 0, start) + 1
        pieces.extend((source[prior:start], replacement))
        prior = end
        records.append({'callee': name, 'argument': index + 1, 'buffer': buffer,
                        'line': line, 'canonicalLine': line + len(used),
                        'before': source[start:end], 'after': replacement})
    revised = (''.join(pieces) + source[prior:]).replace(anchor, helpers + anchor)
    if any(direct_value_arguments(revised, readers)):
        raise ValueError('QUALIFIED_VALUE_ARGUMENT_REMAINS')
    return revised, records
