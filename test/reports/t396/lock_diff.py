import importlib.util
spec=importlib.util.spec_from_file_location('pd',r'D:\hmprojects\HelloWorld\test\t394\pair_display.py')
pd=importlib.util.module_from_spec(spec); spec.loader.exec_module(pd)
from pathlib import Path
a,rn,ap,at,asha,am=pd.load_side(Path(r'D:\hmprojects\HelloWorld\test\reports\t396\lockA.json'))
b,rn2,bp,bt,bsha,bm=pd.load_side(Path(r'D:\hmprojects\HelloWorld\test\reports\t396\lockB.json'))
scr_particle=b'\x91\xa8\x6f'
def analyze(bp_,bt_,rn_,label):
    for i,name in enumerate(['R0','R1','R2']):
        s=bp_[i]; ba=bt_['back'][i]; ca=bt_['candidate'][i]
        if s==ba:
            print(label,name,'scr==back EXACT'); continue
        x0,y0,w,h=rn_[i]
        def pset(img):
            return set((x,y) for y in range(y0,y0+h) for x in range(x0,x0+w)
                       if img[((y-y0)*w+(x-x0))*3:((y-y0)*w+(x-x0))*3+3]==scr_particle)
        S,B=pset(s),pset(ba)
        best=None
        for dy in range(-3,4):
            for dx in range(-3,4):
                moved=set((x+dx,y+dy) for (x,y) in B)
                inter=len(S&moved); err=len(S|moved)-inter
                if best is None or err<best[2]: best=(dx,dy,err,inter)
        print(label,name,'diff; best shift dx=%d dy=%d mismatch=%d inter=%d (scr px %d, back px %d)'%(best[0],best[1],best[2],best[3],len(S),len(B)))
analyze(bp,bt,rn2,'LOCK_B')
# A 侧同样检查
for i,name in enumerate(['R0','R1','R2']):
    s=ap[i]; ba=at['back'][i]
    print('LOCK_A',name,'scr==back',s==ba)
