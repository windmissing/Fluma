"""T665 RX-only lexical binding conversion, after RX injection and before hashing."""
import hashlib
import re

REVISION = 'RX_CAPTURE_SCOPE_HYGIENE_V1'

def digest(text):
    return hashlib.sha256(text.encode('utf8')).hexdigest()

def function_span(source):
    anchor = 'bool g9Capture(){'
    if source.count(anchor) != 1:
        raise ValueError('RX_CAPTURE_FUNCTION_COUNT')
    start = source.index(anchor)
    position = source.index('{', start)
    depth = 0
    # Skip comments/strings so braces there cannot change the binding range.
    token = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|[{}]')
    for match in token.finditer(source, position):
        if match.group() == '{':
            depth += 1
        elif match.group() == '}':
            depth -= 1
            if depth == 0:
                return start, match.end()
    raise ValueError('RX_CAPTURE_UNCLOSED')

def convert(source, contract):
    t=contract['transformation']
    if t['id']!=REVISION or t['expectedFunctions']!=1 or t['expectedTransforms']!=1 or t['names']!={'contactClearLoop':'clearSlot','candidateScanLoop':'scanSlot','selectedPacketLocal':'selectedSlot'}:
        raise ValueError('RX_CAPTURE_TRANSFORMATION_CONTRACT')
    start, end = function_span(source)
    original = source[start:end]
    if digest(original+'\n') != contract['legacyCaptureFunction']['sha256LF']:
        raise ValueError('RX_CAPTURE_LEGACY_FUNCTION_SHA')
    clear = 'for(uint slot=0u;slot<uK;++slot)W[uContacts+slot*16u+15u]=0u;'
    scan = 'for(uint slot=0u;slot<uK;++slot)if(g9Packet(slot,0u)!=0u&&W[uContacts+slot*16u+9u]==4u&&W[uContacts+slot*16u+15u]==0u&&(best==uK||g9CaptureLess(slot,best)))best=slot;'
    selected = 'uint slot=best,'
    if any(original.count(x) != 1 for x in (clear, scan, selected)):
        raise ValueError('RX_CAPTURE_BINDING_COUNT')
    clear_at, scan_at, selected_at = [original.index(x) for x in (clear, scan, selected)]
    if not clear_at < scan_at < selected_at:
        raise ValueError('RX_CAPTURE_BINDING_ORDER')
    bindings = [
        {'name': 'clearSlot', 'begin': clear_at, 'end': clear_at+len(clear)},
        {'name': 'scanSlot', 'begin': scan_at, 'end': scan_at+len(scan)},
        {'name': 'selectedSlot', 'begin': selected_at, 'end': original.rindex('\n }\n return true;')+3},
    ]
    revised = original
    for binding in reversed(bindings):
        a, b = binding['begin'], binding['end']
        piece = re.sub(r'\bslot\b', binding['name'], original[a:b])
        if binding['name'] == 'scanSlot':
            cut = piece.index(')if(')
            piece = piece[:cut+1]+'{'+piece[cut+1:]+'}'
        revised = revised[:a]+piece+revised[b:]
    if re.search(r'\bslot\b', re.sub(r'//[^\n]*|/\*[\s\S]*?\*/','',revised)) or revised.count('uint selectedSlot=best,') != 1:
        raise ValueError('RX_CAPTURE_LEGACY_REMAINS')
    after_bindings=[]
    for binding in bindings:
        name=binding['name'];a=revised.index(('uint ' if name=='selectedSlot' else 'for(uint ')+name)
        if name=='selectedSlot':b=revised.rindex('\n }\n return true;')+3
        elif name=='clearSlot':b=a+len(re.sub(r'\bslot\b',name,clear))
        else:
            piece=re.sub(r'\bslot\b',name,scan);b=a+len(piece)+2
        after_bindings.append({'name':name,'begin':a,'end':b})
    result = source[:start]+revised+source[end:]
    # Only the function range is ever replaced. Prefix/suffix remain byte-identical.
    record = {'revision': REVISION, 'transforms': 1, 'beforeSHA256LF': digest(original+'\n'),
              'afterSHA256LF': digest(revised+'\n'), 'bindingsOriginalFunctionOffsets': bindings,'bindingsCanonicalFunctionOffsets':after_bindings,
              'originalFunctionStart': start, 'originalFunctionEnd': end,
              'firstLine': source[:start].count('\n')+1,
              'lastLine': result[:start+len(revised)].count('\n')+1,
              'sourceBytesUnchangedOutsideFunction': True}
    return result, record
