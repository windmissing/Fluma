"""Bounded G13 archive reader. Structural integrity is not a physics PASS.

No fluid integration, designer checker or device access. Pages are spooled to
an anonymous file; complete fields are never loaded into a Python container.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import tempfile

SEAL=0xc0de1301
ROOT=Path(__file__).resolve().parents[2]

def require(ok,reason):
    if not ok:
        raise ValueError(reason)

def exact_id(value):
    require(isinstance(value,str) and value.isascii() and value.isdigit() and str(int(value))==value and int(value)<2**64,'U64_ID')
    return int(value)

def digest_stream(file,offset,size,mask=None):
    file.seek(offset)
    h=hashlib.sha256()
    at=0
    while at<size:
        data=file.read(min(4096,size-at))
        require(bool(data),'SHORT_RAW')
        if mask is not None:
            data=bytearray(data)
            lo=max(mask[0]-at,0);hi=min(mask[1]-at,len(data))
            if lo<hi:
                data[lo:hi]=bytes(hi-lo)
        h.update(data)
        at+=len(data)
    return h.hexdigest()

def words_at(file,byte_offset,count):
    require(0<=count<=1024,'READ_BUFFER_BOUND')
    file.seek(byte_offset)
    data=file.read(count*4)
    require(len(data)==count*4,'SHORT_WORDS')
    return struct.unpack('<'+str(count)+'I',data)

def pair(w,i):
    return w[i]+(w[i+1]<<32)

def hex_words(w):
    return struct.pack('<'+str(len(w))+'I',*w).hex()

def reserved(file,offset,names):
    for start in range(0,len(names),256):
        w=words_at(file,offset+start*4,min(256,len(names)-start))
        for i,value in enumerate(w,start):
            if names[i].startswith('reserved0'):
                require(value==0,'RESERVED_NONZERO:'+names[i])

def tagged_receipt(file,offset,words,tag):
    header=words_at(file,offset,16)
    require(header[0]==tag and header[1]==1 and header[2]==1 and header[3]==words and header[4]<=4*(words-16) and header[5]==0 and header[14]==0 and header[15]==SEAL,'TAGGED_RECEIPT')
    file.seek(offset+64)
    payload=file.read(header[4])
    require(hashlib.sha256(payload).hexdigest()==hex_words(header[6:14]),'RECEIPT_SHA')
    value=json.loads(payload.decode('utf8'))
    require(isinstance(value,dict),'RECEIPT_OBJECT')
    remaining=4*(words-16)-header[4]
    require(not any(file.read(remaining)),'RECEIPT_ZERO_TAIL')
    return value

def read(path,receipt_path=None):
    table=json.loads((ROOT/'docs/architecture-T607-g13-layout-v1.json').read_text(encoding='utf8'))
    engineering=json.loads((ROOT/'docs/architecture-T607-g13-engineering-v1.json').read_text(encoding='utf8'))
    pages_hash=hashlib.sha256();file_hash=hashlib.sha256();payload_hash=hashlib.sha256()
    offset=0;page_count=0;segment=0;local=0;descriptors=[];header=None;manifest=None;trace=None
    with tempfile.TemporaryFile() as raw, path.open('rb') as source:
        while True:
            line=source.readline(16385)
            if not line:
                break
            require(len(line)<=16384 and line.endswith(b'\n'),'LINE_BOUND_OR_FRAMING')
            file_hash.update(line)
            item=json.loads(line)
            require(isinstance(item,dict),'OBJECT_REQUIRED')
            if item.get('schema')=='G13_ARCHIVE_V1':
                require(manifest is None and not source.read(1),'MANIFEST_LAST')
                manifest=item
                break
            require(item.get('route')==13 and item.get('schema')==1 and item.get('state')=='PAGE','PAGE_SCHEMA')
            page_trace=exact_id(item['id'])
            if trace is None:
                trace=page_trace
            require(trace==page_trace,'TRACE_CHANGED')
            require(type(item['segment']) is int and item['segment']==segment and exact_id(item['wordOffset'])==local and exact_id(item['rawByteOffset'])==offset,'PAGE_ORDER')
            words=item['words']
            require(isinstance(words,list) and 0<len(words)<=1024 and all(type(w) is int and 0<=w<2**32 for w in words),'WORD_BOUND')
            data=struct.pack('<'+str(len(words))+'I',*words)
            if offset==0:
                require(len(words)==1024,'PREFIX_FIRST_PAGE')
                header=words[:256]
                require(header[:4]==[table['magic']['cold'],1,table['magic']['profile'],256] and header[4] in (1,2) and header[5] in (0,1),'COLD_HEADER')
                count=8 if header[4]==2 else 4
                require(header[6]==count and pair(header,8)==trace and 65536<pair(header,26)<=5755136,'DIRECTORY_COUNT_OR_SIZE')
                for i in range(count):
                    d=words[256+32*i:288+32*i]
                    require(d[0]==i+1 and d[1]==1 and d[2]==i and d[3]==0 and d[5]==1 and d[26]==1 and d[31]==SEAL and not any(d[27:31]),'DESCRIPTOR')
                    require(pair(d,6)==sum(pair(x,8) for x in descriptors) and pair(d,8)>0 and pair(d,8)%4==0,'DIRECTORY_OFFSETS')
                    descriptors.append(d)
                require(sum(pair(d,8) for d in descriptors)==pair(header,26),'TOTAL_LENGTH')
            require(descriptors and segment<len(descriptors),'SEGMENT_OVERFLOW')
            d=descriptors[segment]
            require(len(data)<=pair(d,8)-local*4,'SEGMENT_LENGTH')
            raw.write(data);payload_hash.update(data);pages_hash.update(line)
            offset+=len(data);local+=len(words);page_count+=1
            require(page_count<=1+(pair(header,26)+4095)//4096+32,'PAGE_COUNT_BOUND')
            if local*4==pair(d,8):
                segment+=1;local=0
        require(manifest is not None and header is not None and segment==len(descriptors) and local==0,'INCOMPLETE_ARCHIVE')
        require(exact_id(manifest['id'])==trace and manifest['rawBytes']==offset and manifest['pages']==page_count and offset==pair(header,26),'MANIFEST_ID_LENGTH')
        require(payload_hash.hexdigest()==manifest['payloadRawSHA'] and pages_hash.hexdigest()==manifest['pagesFileSHA'],'FILE_OR_PAYLOAD_SHA')
        canonical=digest_stream(raw,0,65536,(248*4,256*4))
        require(canonical==hex_words(header[248:256])==manifest['canonicalPrefixSHA'],'CANONICAL_PREFIX_SHA')
        require(digest_stream(raw,0,65536)==manifest['rawPrefixSHA'],'RAW_PREFIX_SHA')
        for i,d in enumerate(descriptors):
            if i==0:
                require(not any(d[17:25]),'PREFIX_DESCRIPTOR_SHA_SELF_REFERENCE')
            else:
                require(digest_stream(raw,pair(d,6),pair(d,8))==hex_words(d[17:25]),'SEGMENT_SHA:'+str(i))
        profile=next((p for p in table['profiles'] if (p['Nx'],p['Ny'])==(header[96],header[97])),None)
        require(profile is not None,'GRID_PROFILE')
        directory=profile['FULLColdDirectory' if header[4]==2 else 'ROOTColdDirectory']
        for actual,expected in zip(descriptors,directory):
            require(pair(actual,6)==expected['byteOffset'] and pair(actual,8)==expected['byteLength'] and actual[5]==expected['strideWords'],'PROFILE_SEGMENTS')
        reserved(raw,0,table['schemas']['ColdHeader256'])
        require(not any(raw_slice(raw,256*4+32*4*len(descriptors),(32-len(descriptors))*32*4)),'UNUSED_DESCRIPTORS')
        for start,key in ((32,'algorithmSHA'),(40,'layoutSHA'),(48,'gatesSHA'),(56,'mathSHA')):
            require(hex_words(header[start:start+8])==engineering['hashes'][key],'CONTRACT_SHA:'+key)
        config=words_at(raw,1280*4,128)
        require(config[0]==table['magic']['config'] and config[1:4]==(1,table['magic']['profile'],128),'CONFIG_HEADER')
        require((config[4],config[5],config[6])==(header[96],header[97],header[98]),'CONFIG_GEOMETRY')
        masked=list(config);masked[76:84]=[0]*8
        require(hashlib.sha256(struct.pack('<128I',*masked)).hexdigest()==hex_words(config[76:84])==hex_words(header[64:72]),'CONFIG_SHA')
        reserved(raw,1280*4,table['schemas']['Config128'])
        for at,length,tag in ((2432,512,1),(2944,512,2),(3456,1024,3),(4480,256,4),(4992,512,5)):
            tagged_receipt(raw,at*4,length,tag)
        require(not any(raw_slice(raw,6016*4,(16384-6016)*4)),'PREFIX_ZERO_TAIL')
        ctrl=next(d for d,e in zip(descriptors,directory) if e['name']=='CTRL')
        ctrl_start=pair(ctrl,6)
        require(digest_stream(raw,ctrl_start,512)==digest_stream(raw,1280*4,512),'CTRL_CONFIG_BINDING')
        require(digest_stream(raw,ctrl_start+4096,4096)==digest_stream(raw,1408*4,4096),'INPUT_CAPSULE_BINDING')
        permit=words_at(raw,ctrl_start+512,128)
        require(permit[0]==1 and permit[127]==SEAL and pair(permit,8)==pair(header,24),'ROOT_PERMIT')
        require(words_at(raw,5760*4,128)==permit,'PERMIT_ORIGINAL_BINDING')
        base_receipt=tagged_receipt(raw,5888*4,128,7)
        require(hashlib.sha256(base_receipt['acceptedBaseToken'].encode('utf8')).hexdigest()==hex_words(permit[80:88]),'BASE_TOKEN_SHA')
        native=tagged_receipt(raw,5504*4,256,6)
        require(exact_id(native['nonce'])==pair(permit,8) and exact_id(native['deadlineNs'])==pair(permit,38),'NATIVE_ROOT_ID')
        capsule=ctrl_start+4096
        for at,length,tag in ((0,64,8),(320,128,9),(448,128,10),(576,128,11),(704,128,12),(960,64,13)):
            tagged_receipt(raw,capsule+at*4,length,tag)
        original=words_at(raw,capsule+64*4,128);canonical=words_at(raw,capsule+192*4,128)
        for value in (original,canonical):
            require(value[0]==table['magic']['input'] and value[127]==SEAL,'INPUT_HEADER')
            masked=list(value);masked[80:88]=[0]*8
            require(hashlib.sha256(struct.pack('<128I',*masked)).hexdigest()==hex_words(value[80:88]),'INPUT_SELF_SHA')
        allowed={22,23,106,107,*range(80,88)}
        require(all(original[i]==canonical[i] for i in range(128) if i not in allowed),'INPUT_NORMALIZATION')
        require(hex_words(canonical[80:88])==hex_words(permit[72:80])==hex_words(header[80:88]),'INPUT_ROOT_SHA')
        tail=words_at(raw,ctrl_start+73728,256)
        sealed=tail[:4]==(table['magic']['tail'],1,table['magic']['profile'],256) and tail[255]==SEAL and pair(tail,8)==pair(header,24)
        require(header[5] in (0,1) and (header[5]==1 or sealed),'TAIL_OR_FORENSIC')
        if sealed:
            require(tail[39] in (16,820) and tail[40]+tail[41]==tail[39] and tail[37]>>20==0 and tail[38]>>20==0,'TAIL_COUNTS_OR_MASK')
            require(tail[5]==header[100] and tail[7]==header[101],'TAIL_OUTCOME_BINDING')
            if tail[5] in (1,7):
                require((tail[37]&tail[38])==tail[37] and tail[7]==0 and tail[43]==3,'REQUIRED_AUDIT')
            reserved(raw,ctrl_start+73728,table['schemas']['Completion256'])
        if header[5]==0:
            require(native['nativeError']==0 and native['gpuTailAvailable'] and native['nativeOutcome'] in (1,7),'NATIVE_PUBLICATION_OR_DIAGNOSTIC')
        for root in (2048,10240):
            trace_header=words_at(raw,ctrl_start+root*4,64)
            if trace_header[0]:
                require(trace_header[0]==table['magic']['trace'] and trace_header[8:10]==permit[8:10],'TRACE_ROOT_ID')
                if header[5]==0:
                    require(words_at(raw,ctrl_start+(root+1023)*4,1)[0]==SEAL,'TRACE_SEAL')
        for d,e in zip(descriptors,directory):
            if e['name'] not in ('BASE','CANDIDATE','REPLAY'):
                continue
            start=pair(d,6);state=words_at(raw,start,256)
            if state[0]==0 and header[5]==1 and e['name']!='BASE':
                continue
            require(state[:4]==(table['magic']['state'],1,table['magic']['profile'],profile['stateBytes']//4),'STATE_HEADER')
            require(state[4:7]==config[4:7] and state[32:40]==config[76:84] and pair(d,13)==pair(state,16),'STATE_CONFIG_COMMIT_BINDING')
            reserved(raw,start,table['schemas']['StateHeader256'])
        if receipt_path is None:
            receipt_path=Path(str(path)+'.receipt.json')
        require(receipt_path.is_file(),'EXTERNAL_FILE_RECEIPT_REQUIRED')
        if receipt_path:
            receipt=json.loads(receipt_path.read_text(encoding='utf8'))
            require(receipt.get('schema')=='G13_FILE_RECEIPT_V1' and receipt.get('sha256')==file_hash.hexdigest(),'EXTERNAL_FILE_SHA')
        return {'schema':'G13_READER_RECEIPT_V1','state':'STRUCTURALLY_COMPLETE' if header[5]==0 else 'FORENSIC_INCOMPLETE','traceId':str(trace),'rawBytes':offset,'pages':page_count,'fileSHA256':file_hash.hexdigest(),'physicsPass':False,'deviceExecutedByReader':False}

def raw_slice(file,offset,size):
    require(size<=65536,'BUFFER_BOUND')
    file.seek(offset)
    data=file.read(size)
    require(len(data)==size,'SHORT_RAW_SLICE')
    return data

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('archive',type=Path);parser.add_argument('--receipt',type=Path)
    args=parser.parse_args()
    try:
        print(json.dumps(read(args.archive,args.receipt),ensure_ascii=False))
    except (ValueError,KeyError,TypeError,OSError,struct.error) as error:
        print(json.dumps({'schema':'G13_READER_RECEIPT_V1','state':'INVALID_ARCHIVE','reason':str(error),'physicsPass':False}))
        raise SystemExit(1)
