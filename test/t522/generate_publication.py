"""Version the existing publication projection for independent V4 objects."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
GPU=ROOT/'entry/src/main/cpp/gpu_resident'
seal=(GPU/'m1/source_seal.glsl').read_text(encoding='utf-8')
pack=seal[seal.index('void packPublication(){'):seal.index('void sealCompletion(){')]
audit=(GPU/'m1/source_audit.glsl').read_text(encoding='utf-8').split('bool auditPublication(){',1)[1]
audit='bool auditPublication(){'+audit

def once(s,a,b):
    assert s.count(a)==1,a
    return s.replace(a,b)
pack=once(pack,'w=C[uColumn+6u*128u+c],eta=s+w,j=C[uColumn+7u*128u+c],p=C[uColumn+8u*128u+c],r=eta+j+p','w=C[uColumn+6u*128u+c],j=C[uColumn+7u*128u+c],p=C[uColumn+8u*128u+c],eta=s+p+w,r=eta+j')
audit=once(audit,'eta=s+w,r=eta+j+p','eta=s+p+w,r=eta+j')
pack=once(pack,'E[h+1u]=2u','E[h+1u]=4u')
audit=once(audit,'E[h+1u]!=2u','E[h+1u]!=4u')
pack=once(pack,'c,0u,0u);','c,grsQ20FloatBits(s+p),0u);')
audit=once(audit,'c,0u,0u);','c,grsQ20FloatBits(s+p),0u);')
pack=once(pack,'E[h+31u]=1u-uBaseBank','E[h+31u]=uM3CandidateBank')
audit=once(audit,'E[h+31u]!=1u-uBaseBank','E[h+31u]!=uM3CandidateBank')
pack=once(pack,'if(W[S]==4u)return;','if(W[0]!=0u)return;')
audit=once(audit,'if(W[S]==4u)return false;','if(W[0]!=0u)return false;')
audit=once(audit,'W[S+15u]|=512u;return W[S+15u]==1023u;','return true;')
text='''// T522 V4 publication. Source template is retained in M1 history.
uint m3PubWater(){return 16u*uK;}
uint m3PubColumn(){return m3PubWater()+20u*uKw;}
uint m3PubHeader(){return m3PubColumn()+2048u;}
'''+pack+audit
for a,b in [('packPublication','m3PackPublication'),('auditPublication','m3AuditPublication'),('pubWater()','m3PubWater()'),('pubColumn()','m3PubColumn()'),('pubHeader()','m3PubHeader()')]:
    text=text.replace(a,b)
(GPU/'m3a/publication.glsl').write_text(text.rstrip()+'\n',encoding='utf-8')
