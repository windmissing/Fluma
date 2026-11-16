"""Build the V5 SOURCE producer from the retained V4 stages, with semantic
hooks at the real birth/merge cuts. Does not mutate the V4 implementation.
"""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[2]
GPU=ROOT/'entry/src/main/cpp/gpu_resident'

def once(text, old, new):
    assert text.count(old)==1,(old,text.count(old))
    return text.replace(old,new)

def assemble(fragments=None):
    main=(GPU/'m3a/source_v4.comp').read_text(encoding='utf-8-sig')
    def fragment(match):
        name=match[1]
        text=(GPU/name).read_text(encoding='utf-8-sig')
        if name=='m3a/source_events.glsl':
            text=once(text,'row[1]=4u','row[1]=5u')
        if name=='m3a/source_root_audit.glsl':
            text=once(text,'E[at+1u]!=4u','E[at+1u]!=5u')
        if name=='m3a/source_admission.glsl':
            text=once(text,'m3Input(1u)!=4u','m3Input(1u)!=5u')
        if name=='m3a/publication.glsl':
            text=text.replace('E[h+1u]=4u','E[h+1u]=5u').replace('E[h+1u]!=4u','E[h+1u]!=5u')
            text=text.replace('grsQ20FloatBits(s+p),0u','grsQ20FloatBits(s+p),C[uColumn+4u*128u+c]')
            text=once(text,'for(uint i=65u;i<256u;i++)','for(uint i=71u;i<256u;i++)')
            text=once(text,'E[h+41u+i]=C[uHeader+88u+i];',
                      'E[h+41u+i]=C[uHeader+88u+i];for(uint i=0u;i<6u;++i)E[h+65u+i]=C[uHeader+173u+i];')
            text=once(text,'for(uint i=71u;i<256u;i++)',
                      'for(uint i=0u;i<6u;++i)if(E[h+65u+i]!=C[uHeader+173u+i])return false;for(uint i=71u;i<256u;i++)')
        if name=='m3a/source_seed.glsl':
            text=once(text,'m3WriteSand(a,afterA);m3WriteSand(b,afterB);',
                      'if(!v5MergeCertificate(a,b,beforeA[0],beforeB[0])){m3SourceFail(35u,a);return;}m3WriteSand(a,afterA);m3WriteSand(b,afterB);v5RefreshAcceptedMedium();')
            text=once(text,'m3SeedRemaining-=amount;',
                      'uvec2 v5Commit;if(!grsAdd64(uvec2(m3SourceBase(16u),m3SourceBase(17u)),uvec2(1u,0u),v5Commit)||!v5BirthCertificate(slot,0u,v5Commit,m3Get(M3_EVENT_CURSOR),0u,0u)){m3SourceFail(35u,slot);return;}v5RefreshAcceptedMedium();m3SeedRemaining-=amount;')
        if name=='m3a/source_trial_audit.glsl':
            text=text.replace('||id<W[at+7u]||id-W[at+7u]>=W[at+8u]','||!v5CertificateValid(slot,uvec2(m3SourceBase(16u),m3SourceBase(17u)))')
            text=once(text,'return all(equal(sand,expectedSand))',
                      'if(!v5AuditState(false))return false;return all(equal(sand,expectedSand))')
        if name=='m3a/inventory_audit.glsl':
            text=text.replace('!=4u','!=5u').replace('1u,1u,1u,3u,4u,4u,4u,15u,0u','1u,1u,1u,3u,5u,5u,5u,31u,0u').replace('C[uHeader+146u]!=7u','C[uHeader+146u]!=15u')
            text=once(text,'for(uint i=161u;i<256u;++i)','for(uint i=183u;i<256u;++i)')
            text=once(text,'||id<W[event+7u]||id-W[event+7u]>=W[event+8u]',
                      '||(ns==0u?!v5CertificateValid(slot,uvec2(C[uHeader+16u],C[uHeader+17u])):(id<W[event+7u]||id-W[event+7u]>=W[event+8u]))')
            text=once(text,'f!=2u&&f!=6u','f!=2u&&f!=4u&&f!=6u')
            text=once(text,'return sandLive==','if(!v5AuditState(true))return false;return sandLive==')
        if name=='m3a/source_completion.glsl':
            text=text.replace('E[1]=4u','E[1]=5u').replace('!=32767u','!=262143u')
            text=once(text,'memoryBarrierBuffer();E[3]=',
                      'for(uint i=0u;i<22u;++i)E[189u+i]=C[uHeader+161u+i];memoryBarrierBuffer();E[3]=')
        if name=='m3a/source_driver.glsl':
            text=text.replace('M3_AUDIT_FAMILIES,32511u','M3_AUDIT_FAMILIES,261887u')
        if fragments is not None:fragments[name]=text
        return text
    main=re.sub(r'/\* ([A-Za-z0-9_/]+\.glsl) \*/',fragment,main)
    anchor='// Existing M2 resident index and weighted-RNE primitives'
    assert main.count(anchor)==1
    library=(GPU/'m3b/lineage.glsl').read_text(encoding='utf-8')+'\n'+(GPU/'m3b/lineage_audit.glsl').read_text(encoding='utf-8')
    main=main.replace(anchor,library+'\n'+anchor)
    out=ROOT/'.codex/local-test/t517-source.glsl'
    out.write_text(main,encoding='utf-8')
    return out

if __name__=='__main__':
    print(assemble())
