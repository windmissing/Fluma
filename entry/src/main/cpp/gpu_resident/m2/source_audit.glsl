void endSource(){
    uint row[128];
    if(baseTerminal==0u){
        term[0]=admitted==0u?3u:admitted==requestQ?1u:2u;
        if(stopCode!=M2_INPUT_REJECTED)term[1]=stopCode;
        term[3]=admitted;term[4]=requestQ-admitted;term[5]=column;term[6]=births;term[7]=firstId;
        term[8]=initialId;term[9]=h(39u+kind);term[30]=mode;term[31]=h(126u)|(h(127u)<<1u)|4u;
    }
    makeRow(row,M2_END,admitted,0u,0u,0u,0u,0u,admitted,initialId);
    emitSource(row);if(W[S]==4u)return;
    if(admitted!=0u){
        sethp(14u,newSource);sethp(16u,newPhysics);
        uint totals[3]=uint[3](56u+2u*kind,60u+2u*kind,mode==1u?112u:114u);
        for(uint i=0u;i<(kind==0u?2u:3u);++i){
            uvec2 sum;if(!grsAdd64(hp(totals[i]),uvec2(admitted,0u),sum)){fail(13u);return;}sethp(totals[i],sum);
        }
        seth(34u,h(34u)+1u);
    }
    if(appendDedup){seth(33u,h(33u)+1u);seth(74u,h(33u));seth(75u,0u);}
    seth(137u,merges);seth(138u,births);seth(139u,stopCode);seth(140u,128u);seth(141u,cursor());
    sethp(142u,mutation);seth(144u,uEventArena);
    if(baseTerminal==0u){
        for(uint j=0u;j<2u;++j){
            term[10u+j]=h(14u+j);term[12u+j]=h(16u+j);term[14u+j]=admitted==0u?0u:h(14u+j);
            term[16u+j]=B[uHeader+56u+j];term[18u+j]=h(56u+j);term[20u+j]=B[uHeader+58u+j];term[22u+j]=h(58u+j);
            term[24u+j]=h(60u+j);term[26u+j]=h(62u+j);term[28u+j]=h(74u+j);
        }
    }
    uint b=ctrl();
    if(!replaying){
        W[b+5u]=cursor();W[b+6u]=column;W[b+7u]=targetQ;W[b+8u]=requestQ;
        W[b+9u]=admitted;W[b+10u]=0u;W[b+11u]=births;W[b+12u]=merges;W[b+13u]=stopCode;
        W[b+14u]=firstId;W[b+15u]=h(39u+kind);W[b+16u]=newSource.x;W[b+17u]=newSource.y;
        W[b+18u]=uNonce.x;W[b+19u]=uNonce.y;W[b+20u]=mutation.x;W[b+21u]=mutation.y;
        W[b]=chosenIndex;W[b+30u]=2u;
    }else{
        W[b+22u]=chosenIndex;W[b+24u]=admitted;W[b+25u]=0u;W[b+26u]=births;W[b+27u]=merges;
        W[b+28u]=mutation.x;W[b+29u]=mutation.y;W[b+30u]|=4u;
    }
    uint state=baseTerminal!=0u?3u:admitted==0u?2u:1u;
    if(!replaying){
        W[S]=state;W[S+1u]=state==2u?term[1]:0u;W[S+2u]=requestQ;W[S+3u]=admitted;W[S+4u]=births;
        W[S+5u]=column;W[S+6u]=x;W[S+7u]=y;W[S+8u]=mode;W[S+10u]=uint(appendDedup);
        for(uint i=0u;i<32u;++i)W[T+i]=term[i];
    }else{
        if(cursor()!=W[S+9u]||state!=W[S]||uint(appendDedup)!=W[S+10u]){fail(M2_SOURCE_PREFIX_MISMATCH);return;}
        for(uint i=0u;i<32u;++i){W[ctrl()+32u+i]=term[i];if(term[i]!=W[T+i]){uint actualWord=W[T+i];mismatch(M2_SOURCE_PREFIX_MISMATCH,cursor()+1u,i,term[i],actualWord);return;}}
        finalizeV3(true);
    }
}

bool auditOrigins(){
    uint n=h(34u),prefix=h(33u);
    if(n>prefix||prefix>4096u||h(14u)!=n||h(15u)!=0u||h(74u)!=prefix||h(75u)!=0u)return false;
    uvec2 sumS=uvec2(0u),sumW=uvec2(0u),serial=uvec2(0u);
    for(uint i=0u;i<n;++i){
        uint a=sourceEvents()+i*16u,sourceKind=W[a+4u],mass=W[a+6u];
        if(W[a+2u]!=i+1u||W[a+3u]!=0u||sourceKind>1u||mass==0u||mass>CQ||W[a+9u]==0u||W[a+14u]>W[a+8u]||W[a+8u]>(sourceKind==0u?uK:1u)||W[a+15u]>M2_SLOT_LIMIT)return false;
        if(sourceKind==0u){if(!grsAdd64(sumS,uvec2(mass,0u),sumS))return false;}
        else if(!grsAdd64(sumW,uvec2(mass,0u),sumW))return false;
        if(!grsAdd64(serial,uvec2(W[a+8u]+W[a+14u],0u),serial))return false;
        W[origins()+2u*i]=0u;W[origins()+2u*i+1u]=0u;
    }
    if(serial!=hp(142u)){mismatch(M2_SOURCE_MUTATION_SERIAL,uPass==1u?0u:W[S+9u]+1u,serial.x!=h(142u)?142u:143u,serial.x!=h(142u)?serial.x:serial.y,serial.x!=h(142u)?h(142u):h(143u));return false;}
    if(sumS!=hp(60u)||sumW!=hp(62u))return false;
    uint count=0u;
    uvec2 previousPhysics=uvec2(0u);
    for(uint i=0u;i<prefix;++i){
        uint a=DEDUP+i*64u,t=a+32u;
        if(W[a]!=1129534023u||W[a+1u]!=3u||W[a+4u]!=h(4u)||W[a+5u]!=h(5u)||W[t+2u]!=0u||W[t]<1u||W[t]>3u||W[t+31u]<4u||W[t+31u]>7u)return false;
        for(uint j=0u;j<i;++j){uint p=DEDUP+j*64u;if(W[p+6u]==W[a+6u]&&W[p+7u]==W[a+7u])return false;}
        uvec2 physical=uvec2(W[t+12u],W[t+13u]);
        if(grsCompare64(physical,previousPhysics)<0||grsCompare64(physical,hp(16u))>0)return false;
        if(W[t]==3u){
            if(W[t+3u]!=0u||W[t+14u]!=0u||W[t+15u]!=0u||W[t+10u]!=count||W[t+11u]!=0u)return false;
            previousPhysics=physical;continue;
        }
        if(count>=n)return false;
        uint e=sourceEvents()+count*16u;
        if(W[t+14u]!=count+1u||W[t+15u]!=0u||W[e]!=W[a+6u]||W[e+1u]!=W[a+7u]||W[e+4u]!=W[a+2u]||
           W[e+5u]!=W[t+5u]||W[e+6u]!=W[t+3u]||W[e+7u]!=W[t+7u]||W[e+8u]!=W[t+6u]||W[e+9u]!=W[a+14u]||
           W[e+12u]!=W[t+8u]||W[e+13u]!=W[t+9u]||W[e+15u]!=W[t+1u])return false;
        uint rawX,rawY;
        if(!grsFloorBinary64Q20(uvec2(W[a+16u],W[a+17u]),134217727u,rawX)||
           !grsFloorBinary64Q20(uvec2(W[a+18u],W[a+19u]),CQ,rawY)||W[e+10u]!=rawX||W[e+11u]!=rawY||
           W[t+10u]!=count+1u||W[t+11u]!=0u||grsCompare64(physical,previousPhysics)<=0)return false;
        if(W[e+4u]==0u){if(W[t+30u]!=0u||W[t+6u]==0u)return false;}
        else if((W[t+30u]==1u&&(W[t+6u]!=0u||W[t+7u]!=0u||W[t+8u]!=W[t+9u]))||
                (W[t+30u]==2u&&W[t+6u]!=1u)||(W[t+30u]!=1u&&W[t+30u]!=2u))return false;
        previousPhysics=physical;++count;
    }
    return count==n;
}

bool auditState(){
    if(h(0u)!=1213420103u||h(1u)!=3u||h(2u)!=256u||h(31u)!=7u||h(32u)!=0u||h(132u)!=1u||
       h(133u)!=M2_cfg_package||h(134u)!=M2_cfg_lightMax||h(135u)!=M2_cfg_radiusQ||h(136u)!=uK||h(140u)!=128u)return false;
    const uint versions[9]=uint[9](1u,1u,1u,2u,3u,3u,3u,7u,0u);
    for(uint f=0u;f<9u;++f)if(h(24u+f)!=versions[f])return false;
    if(h(122u)!=1u||h(123u)!=65536u||h(124u)!=1u||h(125u)!=1u||h(137u)>h(138u)||h(138u)>uK||
       h(139u)>M2_LEDGER_FULL||h(141u)>2u*uK+1u||h(144u)>1u)return false;
    uint frozen[26]=uint[26](1u,122333u,122333u,244666u,65536u,uK,uK,uKw,uPad,4096u,128u,2u*uK+1u,
        3u,3u,3u,3u,3u,7u,0u,1u,1u,1u,2u,1u,1u,1u);
    for(uint i=0u;i<64u;++i)if(W[config()+i]!=(i<26u?frozen[i]:0u))return false;
    const uint identityFields[6]=uint[6](4u,6u,8u,10u,20u,22u);
    for(uint i=0u;i<6u;++i)for(uint j=0u;j<2u;++j)if(h(identityFields[i]+j)!=B[uHeader+identityFields[i]+j])return false;
    for(uint i=0u;i<24u;++i)if(h(88u+i)!=B[uHeader+88u+i])return false;
    for(uint f=145u;f<256u;++f)if(h(f)!=0u)return false;
    if(int(h(45u))< -7680||int(h(45u))>7680||h(54u)>2621440u||h(55u)!=0u)return false;
    uvec2 expectedPhysics;
    if(!grsAdd64(hp(14u),hp(118u),expectedPhysics)||expectedPhysics!=hp(16u)||!auditOrigins())return false;
    uint liveS=0u,liveW=0u;
    for(uint ns=0u;ns<2u;++ns){
        uint n=ns==0u?uK:uKw,base=ns==0u?0u:uWater,g=ns==0u?10u:11u,next=h(39u+ns),ex=h(126u+ns);
        if(ex>1u||(ns==0u?((next==0u)!=(ex==1u)):(next==0u||(ex==1u&&next!=0xffffffffu))))return false;
        for(uint i=0u;i<n;++i){
            uint row[16];loadSlot(ns,i,row);
            if(row[0]==0u){for(uint f=0u;f<16u;++f)if(f!=g&&row[f]!=0u)return false;continue;}
            if(row[0]>(ns==0u?M2_cfg_mergedMax:CQ)||row[1]>=134217728u||row[2]>CQ||row[5]==0u||row[g]==0u||
               int(row[3])< -1073741824||int(row[3])>1073741824||int(row[4])< -1073741824||int(row[4])>1073741824||
               (next!=0u&&row[5]>=next&&!(ns==1u&&ex==1u&&row[5]==0xffffffffu)))return false;
            for(uint j=i+1u;j<n;++j)if(st(base+j)!=0u&&st(base+5u*n+j)==row[5])return false;
            uint origin=row[ns==0u?8u:6u],sourceId=row[ns==0u?7u:12u],tag=row[ns==0u?6u:10u];
            if(origin==0u||origin>h(34u)||row[ns==0u?9u:7u]!=0u||sourceId==0u||
               (tag&15u)<1u||(tag&15u)>3u||(tag&0xffff0ff0u)!=0u||(tag>>12u)>6u)return false;
            uint e=sourceEvents()+16u*(origin-1u);
            if(W[e+4u]!=ns||W[e+9u]!=sourceId||W[e+7u]==0u||row[5]<W[e+7u]||row[5]-W[e+7u]>=W[e+8u])return false;
            if(ns==0u){
                if(row[11]!=tag>>12u||row[12]!=0u||row[13]!=0u||row[14]>1u)return false;
                uint a=origins()+2u*(origin-1u);uvec2 sum;
                if(!grsAdd64(uvec2(W[a],W[a+1u]),uvec2(row[0],0u),sum)||sum.y!=0u||sum.x>W[e+6u])return false;
                W[a]=sum.x;W[a+1u]=sum.y;++liveS;
            }else{
                if(int(row[4])>0||row[15]!=0u||row[0]>W[e+6u]||grsCompare64(uvec2(row[13],row[14]),hp(118u))>0)return false;
                bool birthCommit=false;
                for(uint j=0u;j<h(33u);++j){uint t=DEDUP+64u*j+32u;
                    if(W[t+14u]==origin&&W[t+15u]==0u){
                        birthCommit=row[8]==W[t+12u]&&row[9]==W[t+13u]&&W[t+30u]==2u;break;
                    }
                }
                if(!birthCommit)return false;
                ++liveW;
            }
        }
    }
    if(h(41u)!=liveS||h(42u)!=liveW||h(43u)!=uK-liveS||h(44u)!=uKw-liveW)return false;
    uvec2 sand=uvec2(0u),water=uvec2(0u),settled=uvec2(0u),flight=uvec2(0u);
    for(uint c=0u;c<128u;++c){
        uint s=col(10u,c),w=col(6u,c),p=0u,j=0u;
        if(col(0u,c)!=0u||col(1u,c)!=0u||col(3u,c)!=0u||col(4u,c)!=0u||col(5u,c)!=0u||col(9u,c)!=0u||s!=col(2u,c)||s>CQ||w>CQ-s)return false;
        for(uint i=0u;i<uK;++i)if(st(uK+i)>>20u==c){uint mass=st(i);if(mass>CQ-p)return false;p+=mass;}
        for(uint i=0u;i<uKw;++i)if(st(uWater+uKw+i)>>20u==c){uint mass=st(uWater+i);if(mass>CQ-j)return false;j+=mass;}
        if(c<127u&&abs(int(col(10u,c+1u))-int(s)-uBaseTangent)>605394)return false;
        if(col(14u,c)>244667u||col(15u,c)>244667u||(col(14u,c)+col(15u,c)!=244667u&&col(14u,c)+col(15u,c)!=0u))return false;
        if(p>CQ-s-w||j>CQ-s-w-p||p!=col(8u,c)||j!=col(7u,c)||s+w!=col(12u,c)||s+w+p+j!=col(13u,c))return false;
        if(!grsAdd64(sand,uvec2(s+p,0u),sand)||!grsAdd64(water,uvec2(w+j,0u),water)||
           !grsAdd64(settled,uvec2(w,0u),settled)||!grsAdd64(flight,uvec2(j,0u),flight))return false;
    }
    return sand==hp(56u)&&sand==hp(60u)&&water==hp(58u)&&water==hp(62u)&&settled==hp(112u)&&flight==hp(114u);
}

void preflight(){
    if((uK!=192u&&uK!=512u)||(uKw!=64u&&uKw!=128u)||uBaseBank>1u||uPublicationSlot>1u||uEventArena>1u||uEventArena!=(uFrame.x&1u)||
       (uNonce.x|uNonce.y)==0u||(uFrame.x|uFrame.y)==0u||(uControl.x|uControl.y)==0u){failAt(9u,0u,0u);return;}
    if(!auditState()){failAt(15u,0u,0u);return;}
    W[ctrl()+1u]=1u;
    if(W[0]!=1129534023u||W[1]!=3u||W[4]!=h(4u)||W[5]!=h(5u)){failAt(9u,0u,0u);return;}
}

void finalizeV3(bool verify){
    uint n=B[uHeader+34u],d=B[uHeader+33u];
    if(W[S]==1u){
        if(n>=4096u){fail(11u);return;}
        uint a=sourceEvents()+n*16u;
        uint words[16]=uint[16](W[6],W[7],h(14u),h(15u),W[2],W[S+5u],W[S+3u],W[T+7u],W[T+6u],W[14],W[S+6u],W[S+7u],W[T+8u],W[T+9u],h(137u),h(139u));
        if(verify)words=uint[16](W[6],W[7],h(14u),h(15u),kind,column,admitted,term[7],births,W[14],x,y,term[8],term[9],merges,stopCode);
        for(uint i=0u;i<16u;++i){if(verify&&W[a+i]!=words[i]){uint actualWord=W[a+i];mismatch(M2_SOURCE_PREFIX_MISMATCH,cursor()+1u,a+i,words[i],actualWord);return;}if(!verify)W[a+i]=words[i];}
    }
    if(W[S+10u]!=0u){
        if(d>=4096u){fail(11u);return;}
        uint a=DEDUP+64u*d;
        for(uint i=0u;i<32u;++i){
            if(verify&&(W[a+i]!=W[i]||W[a+32u+i]!=term[i])){
                uint expectedWord=W[a+i]!=W[i]?W[i]:term[i];
                uint actualWord=W[a+i]!=W[i]?W[a+i]:W[a+32u+i];
                mismatch(M2_SOURCE_PREFIX_MISMATCH,cursor()+1u,a+i,expectedWord,actualWord);return;
            }
            if(!verify){W[a+i]=W[i];W[a+32u+i]=W[T+i];}
        }
    }
    uvec2 generation;
    if(!grsAdd64(hp(72u),uvec2(1u,0u),generation)){fail(13u);return;}
    sethp(72u,generation);sethp(12u,uNonce);sethp(18u,uControl);sethp(49u,uFrame);
    uint sourceState=W[S];
    seth(76u,1u-uBaseBank);seth(77u,uBaseBank);seth(78u,2u);seth(79u,1u);seth(80u,sourceState);seth(81u,3u);seth(47u,uCarry);seth(38u,0u);
    if(!auditState()){fail(15u);return;}
    if(!verify)W[S+15u]=255u;
}

void compareSource(){
    for(uint i=0u;i<uWords;++i){if(W[spare()+i]!=C[i]){uint expectedWord=W[spare()+i],actualWord=C[i];mismatch(i==uHeader+142u||i==uHeader+143u?M2_SOURCE_MUTATION_SERIAL:M2_SOURCE_PREFIX_MISMATCH,W[S+9u]+1u,i,expectedWord,actualWord);return;}}
    W[S+12u]=1u;W[S+15u]|=256u;
}
