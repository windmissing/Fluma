"""Check the actual assembled shader boundary that failed on the device."""
import hashlib
import json
import re
import sys
from pathlib import Path


def direct_storage_arguments(source):
    names = set(re.findall(r'\b(?:void|bool|uint|uvec[234]|int)\s+(\w+)\s*\(', source))
    violations = []
    for match in re.finditer(r'\b(\w+)\s*\(', source):
        if match[1] not in names:
            continue
        depth = 0
        start = match.end()
        arguments = []
        for index in range(start, len(source)):
            char = source[index]
            if char in '([':
                depth += 1
            if char == ')' and depth == 0:
                arguments.append(source[start:index].strip())
                break
            if char in ')]':
                depth -= 1
            if char == ',' and depth == 0:
                arguments.append(source[start:index].strip())
                start = index + 1
        if any(re.fullmatch(r'[CWE]\[.*\]', arg, re.S) for arg in arguments):
            violations.append({'line': source.count('\n', 0, match.start()) + 1,
                               'function': match[1]})
    return violations


if __name__ == '__main__':
    old_project, new_project = map(Path, sys.argv[1:3])
    old = (old_project / 'source-v3.glsl').read_bytes()
    new = (new_project / 'source-v3.glsl').read_bytes()
    original = old.decode()
    revised = new.decode()
    failures = direct_storage_arguments(original)
    assert {'line': 1138, 'function': 'm2KeyLess'} in failures
    assert not direct_storage_arguments(revised)
    for qualifier in ['coherent', 'volatile', 'restrict']:
        pattern = r'[^\n]*\b' + qualifier + r'\s+buffer[^\n]*'
        assert re.findall(pattern, original) == re.findall(pattern, revised)
    assert original.count('memoryBarrierBuffer();') == revised.count('memoryBarrierBuffer();')
    assert original.count('barrier();') == revised.count('barrier();')
    assert (old_project / 'quantum-v3.glsl').read_bytes() == (new_project / 'quantum-v3.glsl').read_bytes()
    before = hashlib.sha256(old).hexdigest()
    after = hashlib.sha256(new).hexdigest()
    assert before == 'eb479a7b4fb3be0dc06245aef95b05e4ceabcaa9f9de7051994c5ac4504a1c1c'
    assert before != after
    print(json.dumps({'schema': 'T482_SHADER_BOUNDARY_CHECK_V1', 'passed': 8,
                      'failed': 0, 'oldSourceSHA': before, 'newSourceSHA': after,
                      'oldViolations': failures, 'newViolations': [],
                      'barriersPreserved': True, 'qualifiersPreserved': True,
                      'finalQuantumUnchanged': True, 'device': 'NOT_RUN'}))
