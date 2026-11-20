"""Specialize the reviewed G13 entry and emit its transitive GLSL functions."""
import re
from collections import defaultdict

LIMIT = 48 * 1024
TOKEN = re.compile(r'[A-Za-z_]\w*|0[xX][0-9a-fA-F]+u?|\d+u?|&&|\|\||==|!=|<=|>=|[^\s]')
IDENT = re.compile(r'\b[A-Za-z_]\w*\b')
CALL = re.compile(r'\b([A-Za-z_]\w*)\s*\(')


def matching(tokens):
    pairs, stack = {}, []
    closes = {')': '(', ']': '[', '}': '{'}
    for i, token in enumerate(tokens):
        if token in ('(', '[', '{'):
            stack.append(i)
        elif token in closes:
            if not stack or tokens[stack[-1]] != closes[token]:
                raise ValueError('Unbalanced G13 source')
            pairs[stack.pop()] = i
    if stack:
        raise ValueError('Unclosed G13 source')
    return pairs


def condition(tokens, op, constants):
    """Only reduce boolean expressions over literals and the dispatch opcode."""
    if '||' in tokens and '(' not in tokens:
        for part in split_on(tokens, '||'):
            value = condition(part, op, constants)
            if value is not False:
                return value
        return False
    if '&&' in tokens and '(' not in tokens:
        for part in split_on(tokens, '&&'):
            value = condition(part, op, constants)
            if value is not True:
                return value
        return True
    if len(tokens) != 3:
        return None
    values = dict(constants, uOp=op)
    operands = [values.get(key, int(key[:-1]) if re.fullmatch(r'\d+u', key) else None)
                for key in (tokens[0], tokens[2])]
    if None in operands:
        return None
    left, right = operands
    return {'==': left == right, '!=': left != right, '<': left < right,
            '>': left > right, '<=': left <= right, '>=': left >= right}.get(tokens[1])


def split_on(tokens, separator):
    start = 0
    for i, token in enumerate(tokens):
        if token == separator:
            yield tokens[start:i]
            start = i + 1
    yield tokens[start:]


def specialize(main, op, constants):
    matches = list(TOKEN.finditer(main))
    tokens = [match[0] for match in matches]
    pairs = matching(tokens)
    def original(start, end):
        return main[matches[start].start():matches[end-1].end()]

    def statement(start):
        token = tokens[start]
        if token == '{':
            end = pairs[start]
            body, returns = block(start + 1, end)
            return end + 1, '{' + body + '}', returns
        if token == 'if':
            if tokens[start+1] != '(':
                raise ValueError('Expected G13 if condition')
            close = pairs[start+1]
            result = condition(tokens[start+2:close], op, constants)
            end, yes, yes_returns = statement(close+1)
            no, no_returns = '', False
            if end < len(tokens) and tokens[end] == 'else':
                end, no, no_returns = statement(end+1)
            if result is not None:
                selected = yes if result else no
                return end, '{' + selected + '}' if selected else '', yes_returns if result else no_returns
            # An empty specialized arm still owns its original dangling else.
            return end, original(start, close+1) + (yes or '{}') + (' else ' + no if no else ''), yes_returns and no_returns
        if token in ('for', 'while', 'switch', 'do'):
            if token not in ('for', 'while'):
                raise ValueError('Unsupported G13 entry control flow: ' + token)
            close = pairs[start+1]
            end, body, _ = statement(close+1)
            return end, original(start, close+1) + (body or '{}'), False
        end = start
        while tokens[end] != ';':
            if tokens[end] in ('}', '{'):
                raise ValueError('Unsupported G13 entry statement')
            end = pairs.get(end, end) + 1
        return end+1, original(start, end+1), token == 'return'

    def block(start, end):
        statements = []
        while start < end:
            start, text, returns = statement(start)
            if text:
                statements.append(text)
            if returns:
                return '\n'.join(statements), True
        return '\n'.join(statements), False

    if tokens[:4] != ['void', 'main', '(', ')'] or pairs[4] != len(tokens)-1:
        raise ValueError('Unexpected G13 main entry')
    return block(5, len(tokens)-1)[0]


def source_units(source, hooks):
    source = re.sub(r'/\*.*?\*/|//[^\n]*', lambda m: '\n' * m[0].count('\n') or ' ', source, flags=re.S)
    source = re.sub(r'^#ifdef G13_TEST_HOOKS\s*\n(.*?)^#endif[^\n]*',
                    lambda m: m[1] if hooks else '', source, flags=re.M | re.S)
    directives = re.findall(r'^#define (\w+) ([^\n]+)$', source, re.M)
    definitions = dict(directives)
    if len(definitions) != len(directives):
        raise ValueError('Duplicate G13 macro')
    source = re.sub(r'^#(?:version|define) [^\n]*\n', '', source, flags=re.M)
    if re.search(r'^\s*#', source, re.M):
        raise ValueError('Unsupported G13 shader directive')
    matches = list(TOKEN.finditer(source))
    tokens = [match[0] for match in matches]
    pairs = matching(tokens)
    units = []
    start = at = 0
    while at < len(tokens):
        token = tokens[at]
        if token == '{' and at >= start+4 and tokens[at-1] == ')':
            name = tokens[start+1]
            if tokens[start+2] != '(':
                raise ValueError('Unsupported G13 top-level function signature')
            end = pairs[at]
            units.append((name, source[matches[start].start():matches[end].end()]))
            at = start = end+1
        elif token == ';':
            units.append((None, source[matches[start].start():matches[at].end()]))
            at = start = at+1
        else:
            at = pairs.get(at, at)+1
    if start != len(tokens):
        raise ValueError('Unexpected trailing G13 source')
    return definitions, units


def roles(source, dispatch, hooks=False):
    definitions, units = source_units(source, hooks)
    functions = defaultdict(list)
    for name, text in units:
        if name:
            functions[name].append(text)
    main, = functions.pop('main')
    constants = {f'OP_{name}': value for name, value in dispatch['opcodes'].items()}
    if any(definitions[name] != str(value)+'u' for name, value in constants.items()):
        raise ValueError('G13 opcode declaration mismatch')
    ops = sorted(set(constants.values()) | {64, 65, 66})
    bodies = {op: specialize(main, op, constants) for op in ops}

    def emit(selected):
        # Every linked program must execute both original FP32 probe groups.
        # Only Boot owns normal opcode 66 dispatch; other programs use it
        # privately during preparation, before issuing any root permit.
        entry = (66, *(op for op in selected if op != 66))
        body = '\n'.join(f'if(uOp=={op}u){{\n{bodies[op]}\nreturn;\n}}' for op in entry)
        names = set()
        remaining = list(CALL.findall(body))
        while remaining:
            name = remaining.pop()
            if name in functions and name not in names:
                names.add(name)
                for code in functions[name]:
                    remaining.extend(CALL.findall(code))
        # Global declarations preserve all original buffer layouts and shared
        # scratch. The compiler may optimize truly unused uniform locations.
        chunks = [text for name, text in units if name is None or name in names]
        chunks.append('void main(){\n'+body+'\n}')
        references = set(IDENT.findall('\n'.join(chunks)))
        before = ['#version 310 es']
        for key, value in definitions.items():
            if key in references:
                before.append(f'#define {key} {value}')
        return ('\n'.join(before+chunks)+'\n').encode(), sorted(names)

    result = [('BOOT', (64,65,66))]
    # Group opcode arms that share reachable functions, staying inside the
    # size gate. Stable order gives repeatable role/cache identity.
    groups = []
    for op in range(64):
        solo, _ = emit((op,))
        if len(solo) > LIMIT:
            raise ValueError(f'G13 opcode {op} closure {len(solo)} exceeds {LIMIT}')
        best = None
        for i, (group, length) in enumerate(groups):
            candidate, _ = emit((*group, op))
            if len(candidate) <= LIMIT:
                score = len(solo) + length - len(candidate)
                if best is None or score > best[0]:
                    best = (score, i, len(candidate))
        if best is None:
            groups.append(((op,), len(solo)))
        else:
            _, index, length = best
            groups[index] = ((*groups[index][0], op), length)
    result.extend((f'OP_{group[0]:02d}', group) for group, _ in groups)
    for name, group in result:
        compiled, dependencies = emit(group)
        if len(compiled) > LIMIT:
            raise ValueError(f'G13 role {name} closure {len(compiled)} exceeds {LIMIT}')
        yield name, group, compiled, dependencies
