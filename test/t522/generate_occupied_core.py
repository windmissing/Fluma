"""Version the existing GPU motion core for the frozen M3 occupied surface.

Reuses integer/T16/oracle assets; never edits or reroutes M1/M2. Exact-anchor
assertions make a changed upstream contract a generation error.
"""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
GPU=ROOT/'entry/src/main/cpp/gpu_resident'
def read(name):return (GPU/name).read_text(encoding='utf-8-sig')
def once(text,old,new):
    assert text.count(old)==1,(old,text.count(old))
    return text.replace(old,new)
def generate():
    core=read('m1/quantum_core.comp')
    core=core[:core.index('void main(){')]
    core=once(core,'uint eta(uint c){return col(10u,c)+col(6u,c);}uint occupied(uint c){return eta(c)+col(7u,c)+col(8u,c);}',
        'uint eta(uint c){return col(10u,c)+col(8u,c)+col(6u,c);}uint occupied(uint c){return eta(c)+col(7u,c);}')
    core=once(core,'void derived(uint c){pc(12u,c,eta(c));pc(13u,c,occupied(c));}',
        'void derived(uint c){pc(11u,c,col(10u,c)+col(8u,c));pc(12u,c,eta(c));pc(13u,c,occupied(c));}')
    core=once(core,'e[1]=2u','e[1]=4u')
    core=once(core,'e[32]=a0;', 'e[52]|=uM3Role<<22u;e[32]=a0;')
    # Declare before emit(): role bits are semantic metadata, not a time scale.
    core=once(core,'uniform uint uK,','#define M3_SCOPE_KERNEL\n#define M3_ROLE_DEFINED\nuniform uint uM3Role;\nuniform uint uK,')
    core=once(core,'// W0 first error,','''#ifndef M3_EVENT_WORD_DEFINED
#define M3_EVENT_WORD_DEFINED
uniform uint uM3EventWord;
#endif
uniform uint uM3RootPass;
// W0 first error,''')
    core=once(core,'W[2]=uPass;','W[2]=uM3RootPass+1u;')
    core=core.replace('E[row*64u+i]','E[uM3EventWord+row*64u+i]')
    core=once(core,'uniform int uRequestedAngle;','int m3ScopeAngle;')
    core=core.replace('uRequestedAngle','m3ScopeAngle')
    core=once(core,'/* INTEGER_PRIMITIVES */',read('integer_primitives.glsl'))
    # Masks 3..14 retain the established operand fields, including flags52.
    mask=read('m1/quantum_event_masks.glsl')
    core=once(core,'/* EVENT_MASKS */',mask)
    core=once(core,'/* T16 */',read('m1/quantum_t16.glsl'))
    # Definitions precede motion: no capacity writer may ignore existing J.
    capacity='''
uint m3JClearance(uint c){
 uint result=CQ-col(7u,c);
 for(uint i=0u;i<uKw;++i)if(C[uWater+i]!=0u&&(C[uWater+uKw+i]>>20u)==c){uint y=C[uWater+2u*uKw+i];result=min(result,y==0u?0u:y-1u);}
 return result>eta(c)?result-eta(c):0u;
}
'''
    motion=read('m1/quantum_motion.glsl')
    motion=once(motion,'capacity=mass>CQ-occupied(to)', 'capacity=mass>CQ-occupied(to)||(ns==0u&&mass>m3JClearance(to))')
    core=once(core,'/* MOTION */',capacity+motion)
    stages=read('m1/quantum_stages.glsl')
    stages=once(stages,'C[uHeader+1u]!=2u','C[uHeader+1u]!=4u')
    stages=once(stages,'C[uHeader+31u]!=3u','C[uHeader+31u]!=15u')
    stages=once(stages,'||col(11u,c)!=0u','')
    # inventory() is a physical precheck here. The full V4 ledger/metadata and
    # final live/free summaries are independently audited by scope finalize.
    stages=once(stages,'col(12u,c)!=s+w||col(13u,c)!=s+w+p+j', 'col(11u,c)!=s+p||col(12u,c)!=s+p+w||col(13u,c)!=s+w+p+j')
    stages=once(stages,'if(W[se+4u]!=ns||','if((W[se+4u]==1u?1u:0u)!=ns||')
    stages=once(stages,'min(CQ-occupied(r),min(col(15u,d),col(15u,r)))', 'min(min(CQ-occupied(r),m3JClearance(r)),min(col(15u,d),col(15u,r)))')
    start=stages.index(' int low=-605394,high=605394;')
    end=stages.index('uvec2 serial;',start)
    # The GPU planner already froze and entry-checked this role's angle. Do
    # not silently clamp the REQUESTED role using the former M1 policy.
    stages=stages[:start]+''' int accepted=uRequestedAngle;for(uint c=0u;c<127u;++c)if(abs(int(col(10u,c+1u))-int(col(10u,c))-int(W[uAngles+uint(accepted+7680)*4u]))>605394){qfail(14u);return;}
 W[4]=uint(accepted);W[5]=W[uAngles+uint(accepted+7680)*4u];'''+stages[end:]
    core=once(core,'/* STAGES */',stages)
    core=core.replace('uRequestedAngle','m3ScopeAngle')
    (GPU/'m3a/occupied_core.glsl').write_text(core.rstrip()+'\n',encoding='utf-8')
if __name__=='__main__':generate()
