// Per-invocation workgroup variables are used only by lane zero. Pair and
// index decisions that change execution paths are shared across all lanes.
shared uint stepCode,slotA,slotB,mutate,chosenIndex;
const uint STEP_STOP=0u,STEP_BIRTH=1u,STEP_PAIR=2u,STEP_DIRECT=3u;
uint admitted=0u,births=0u,merges=0u,firstId=0u,initialId=0u,stopCode=0u;
uint requestQ=0u,targetQ=0u,kind=0u,mode=0u,x=0u,y=0u,column=0u;
uint baseTerminal=0u;
bool appendDedup=false;
uvec2 newSource,newPhysics,mutation;
uint term[32];
uint stockField(){return mode==0u?8u:mode==1u?6u:7u;}
uint stock(){return col(stockField(),column);}
void derived(){
    uint eta=col(10u,column)+col(6u,column);
    setcol(12u,column,eta);setcol(13u,column,eta+col(7u,column)+col(8u,column));
}
void makeRow(out uint row[128],uint eventKind,uint q,uint a,uint b,uint stockBefore,uint stockAfter,uint beforeQ,uint afterQ,uint oldId){
    for(uint i=0u;i<128u;++i)row[i]=0u;
    row[0]=M2_sourceMagic;row[1]=3u;row[2]=eventKind;row[3]=1u;
    row[4]=uNonce.x;row[5]=uNonce.y;row[6]=cursor()+1u;row[8]=W[6];row[9]=W[7];
    row[10]=newSource.x;row[11]=newSource.y;row[12]=newPhysics.x;row[13]=newPhysics.y;
    row[18]=q;row[19]=beforeQ;row[21]=afterQ;row[25]=births;row[26]=merges;
    row[28]=oldId;row[29]=h(39u+kind);row[30]=h(126u)|(h(127u)<<1u)|4u;
    if(eventKind==M2_END)row[27]=stopCode;
    else{
        row[14]=kind;row[17]=column;row[23]=stockBefore;row[24]=stockAfter;
        if(eventKind!=M2_DIRECT)row[15]=a;
        if(eventKind==M2_MERGE)row[16]=b;
    }
}
void emitSource(uint row[128]){
    uint at=cursor();
    if(!replaying){
        if(!m2SealSourceRow(row,at,2u*uK+1u)){failAt(M2_SOURCE_EVENT_SCHEMA,at+1u,0u);return;}
    }else{
        uint actual[128];
        uint sealedRows=W[S+9u];
        if(!m2ReadSourceRow(at,sealedRows,2u*uK+1u,actual)){
            uint word=31u,expected=M2_sourceSeal;
            if(E[at*128u+31u]==M2_sourceSeal){
                for(uint i=0u;i<128u;++i){uint v=i==31u?M2_sourceSeal:row[i];
                    if(actual[i]!=v){word=i;expected=v;break;}
                }
            }
            uint actualWord=E[at*128u+word];
            mismatch(M2_SOURCE_EVENT_SCHEMA,at+1u,word,expected,actualWord);return;
        }
        const uint orderFields[8]=uint[8](2u,15u,16u,25u,26u,27u,28u,29u);
        for(uint f=0u;f<8u;++f){uint i=orderFields[f];if(actual[i]!=row[i]){
            mismatch(M2_SOURCE_ORDER,at+1u,i,row[i],actual[i]);return;
        }}
        row[31]=M2_sourceSeal;
        for(uint i=0u;i<128u;++i)if(row[i]!=actual[i]){mismatch(M2_SOURCE_PREFIX_MISMATCH,at+1u,i,row[i],actual[i]);return;}
    }
    nextRow();
}
void rejectInput(uint why){stopCode=M2_INPUT_REJECTED;term[1]=65536u|why;}
void decodeSource(){
    admitted=0u;births=0u;merges=0u;firstId=0u;baseTerminal=0u;appendDedup=false;
    requestQ=0u;targetQ=0u;kind=0u;mode=0u;column=0u;x=0u;y=0u;
    stopCode=M2_TARGET_COMPLETE;mutation=hp(142u);
    for(uint j=0u;j<32u;++j)term[j]=0u;
    newSource=hp(14u);newPhysics=hp(16u);
    uint prefix=h(33u);
    if(W[2]<=1u)kind=W[2];
    if(W[0]!=1129534023u||W[1]!=3u||W[4]!=h(4u)||W[5]!=h(5u)||W[31]!=0u){rejectInput(9u);return;}
    for(uint i=0u;i<prefix;++i){
        uint a=DEDUP+64u*i;
        if(W[a+4u]!=W[4]||W[a+5u]!=W[5]||W[a+6u]!=W[6]||W[a+7u]!=W[7])continue;
        for(uint j=0u;j<32u;++j)if(W[a+j]!=W[j]){stopCode=M2_PAYLOAD_CONFLICT;return;}
        baseTerminal=a+32u;
        for(uint j=0u;j<32u;++j)term[j]=W[baseTerminal+j];
        term[2]=1u;mode=term[30];stopCode=M2_DEDUP_REPLAY;return;
    }
    if(prefix==4096u){stopCode=M2_LEDGER_FULL;return;}
    appendDedup=true;
    if(W[2]>1u||W[3]!=0u||W[30]!=0u){rejectInput(1u);return;}
    kind=W[2];
    if(W[8]!=h(84u)||W[9]!=h(85u)||W[12]!=h(20u)||W[13]!=h(21u)||W[28]!=h(22u)||W[29]!=h(23u)||
       W[14]==0u||W[15]==0u||(W[6]|W[7])==0u||(W[10]|W[11])==0u){rejectInput(9u);return;}
    if((W[24]|W[25])==0u||W[27]>1u){rejectInput(10u);return;}
    uint quantity;
    if(!grsFloorBinary64Q20(uvec2(W[16],W[17]),134217727u,x)||!grsFloorBinary64Q20(uvec2(W[18],W[19]),CQ,y)||
       !grsFloorBinary64Q20(uvec2(W[20],W[21]),CQ,quantity)||
       (W[19]&0x7fffffffu)>0x40700000u||((W[19]&0x7fffffffu)==0x40700000u&&W[18]!=0u)||
       (W[21]&0x7fffffffu)>0x40700000u||((W[21]&0x7fffffffu)==0x40700000u&&W[20]!=0u)){rejectInput(2u);return;}
    requestQ=quantity;column=x>>20u;
    if(quantity==0u){rejectInput(3u);return;}
    if(y<col(10u,column)){rejectInput(4u);return;}
    mode=kind==0u?0u:y<=col(12u,column)?1u:2u;
    targetQ=min(quantity,CQ-col(13u,column));
}
void bumpMutation(){
    uvec2 next;
    if(!grsAdd64(mutation,uvec2(1u,0u),next))failAt(M2_SOURCE_MUTATION_SERIAL,cursor()+1u,142u);
    else mutation=next;
}
bool preBirth(uint slot){
    uint row[16];loadSlot(kind,slot,row);
    if(row[kind==0u?10u:11u]==0xffffffffu){failAt(21u,cursor()+1u,slot);return false;}
    return true;
}
void startPhysical(){
    if(admitted!=0u)return;
    if(!grsAdd64(hp(14u),uvec2(1u,0u),newSource)||!grsAdd64(hp(16u),uvec2(1u,0u),newPhysics))failAt(13u,cursor()+1u,14u);
}
void mergeSelected(uint a,uint b){
    uint oldA[16],oldB[16],outA[16],outB[16],row[128];
    m2LoadSand(a,oldA);m2LoadSand(b,oldB);
    uint c=oldA[1]>>20u;
    if(!m2MergeRows(oldA,oldB,M2_cfg_radiusQ,col(10u,c),col(12u,c),col(6u,c),outA,outB)){
        failAt(M2_SOURCE_ORDER,cursor()+1u,a);return;
    }
    bumpMutation();if(W[S]==4u)return;
    putSlot(0u,a,outA);putSlot(0u,b,outB);seth(41u,h(41u)-1u);seth(43u,h(43u)+1u);++merges;
    makeRow(row,M2_MERGE,oldB[0],a,b,col(8u,c),col(8u,c),admitted,admitted,h(39u));row[17]=c;
    for(uint f=0u;f<16u;++f){row[32u+f]=oldA[f];row[48u+f]=oldB[f];row[64u+f]=outA[f];row[80u+f]=outB[f];}
    emitSource(row);
}
void birthSelected(uint slot){
    uint old[16],outRow[16],row[128];loadSlot(kind,slot,old);
    if(old[0]!=0u||!preBirth(slot)){failAt(M2_SOURCE_ORDER,cursor()+1u,slot);return;}
    bumpMutation();if(W[S]==4u)return;
    uint mass=mode==0u?min(M2_cfg_package,targetQ-admitted):targetQ;
    uint id=h(39u+kind),generation=old[kind==0u?10u:11u]+1u;
    for(uint i=0u;i<16u;++i)outRow[i]=0u;
    outRow[0]=mass;outRow[1]=x;outRow[2]=y;outRow[5]=id;
    if(kind==0u){
        outRow[6]=1u;outRow[7]=W[14];outRow[8]=newSource.x;outRow[9]=newSource.y;outRow[10]=generation;
        outRow[14]=col(6u,column)>0u&&y>col(10u,column)&&y<=col(12u,column)?1u:0u;
    }else{
        outRow[4]=0u-uEmitQ;outRow[6]=newSource.x;outRow[7]=newSource.y;
        outRow[8]=newPhysics.x;outRow[9]=newPhysics.y;outRow[10]=1u;outRow[11]=generation;outRow[12]=W[14];
        outRow[13]=h(118u);outRow[14]=h(119u);
    }
    putSlot(kind,slot,outRow);seth(41u+kind,h(41u+kind)+1u);seth(43u+kind,h(43u+kind)-1u);
    seth(39u+kind,id==0xffffffffu&&kind==1u?id:id+1u);
    if(id==0xffffffffu)seth(126u+kind,1u);
    if(births==0u)firstId=id;
    ++births;
    uint oldStock=stock();setcol(stockField(),column,oldStock+mass);derived();
    makeRow(row,M2_BIRTH,mass,slot,0u,oldStock,oldStock+mass,admitted,admitted+mass,id);
    for(uint i=0u;i<16u;++i){row[32u+i]=old[i];row[64u+i]=outRow[i];}
    emitSource(row);admitted+=mass;
}
void direct(){
    uint row[128],old=stock();setcol(6u,column,old+targetQ);derived();
    makeRow(row,M2_DIRECT,targetQ,0u,0u,old,old+targetQ,0u,targetQ,h(40u));emitSource(row);admitted=targetQ;
}
void decide(){
    stepCode=STEP_STOP;
    if(W[S]==4u||stopCode!=M2_TARGET_COMPLETE)return;
    if(admitted==requestQ)return;
    if(admitted==targetQ){stopCode=M2_COLUMN_LIMIT;return;}
    if(mode==1u){stepCode=STEP_DIRECT;return;}
    if(births==uK){stopCode=M2_BIRTH_LIMIT;return;}
    if(h(126u+kind)!=0u||h(39u+kind)==0u){stopCode=M2_ID_EXHAUSTED;return;}
    uint count=kind==0u?uK:uKw,base=kind==0u?0u:uWater;
    for(uint i=0u;i<count;++i)if(st(base+i)==0u){slotA=i;stepCode=STEP_BIRTH;return;}
    if(kind==1u){stopCode=M2_SLOT_LIMIT;return;}
    stepCode=STEP_PAIR;
}
void endSource();
void finalizeV3(bool verify);
void executeSource(){
    uint lane=gl_LocalInvocationIndex;
    if(lane==0u){decodeSource();initialId=h(39u+kind);chosenIndex=index0();if(replaying)W[controlBase()]=0u;}
    memoryBarrierBuffer();barrier();
    for(uint round=0u;round<=uK;++round){
        if(lane==0u)decide();barrier();
        if(stepCode==STEP_STOP)break;
        if(stepCode==STEP_PAIR){
            uvec2 pair=m2IndexPair(chosenIndex,uPad,uK,M2_cfg_radiusQ);
            if(lane==0u){
                if(pair.x==uK){stopCode=M2_NO_PAIR;stepCode=STEP_STOP;}
                else if(pair.y>=uK||!preBirth(pair.y)){failAt(M2_SOURCE_ORDER,cursor()+1u,pair.y);stepCode=STEP_STOP;}
                else {slotA=pair.x;slotB=pair.y;}
            }
            memoryBarrierBuffer();barrier();
            if(stepCode==STEP_STOP)break;
        }
        if(lane==0u){
            uint rows=stepCode==STEP_PAIR?3u:2u;
            if(cursor()>2u*uK+1u-rows){failAt(17u,cursor()+1u,0u);}
            else {startPhysical();if(W[S]!=4u&&stepCode==STEP_PAIR)mergeSelected(slotA,slotB);}
        }
        memoryBarrierBuffer();barrier();
        if(W[S]==4u)break;
        if(stepCode==STEP_PAIR){
            uint dst=2u*index0()+uPad-chosenIndex;
            m2IndexUpdate(chosenIndex,dst,uPad,uK,slotA,slotB);
            if(lane==0u){chosenIndex=dst;slotA=slotB;}
            barrier();
        }
        if(lane==0u){
            if(stepCode==STEP_DIRECT)direct();else birthSelected(slotA);
            mutate=kind==0u&&W[S]!=4u?1u:0u;
        }
        memoryBarrierBuffer();barrier();
        if(mutate!=0u){
            uint dst=2u*index0()+uPad-chosenIndex;
            m2IndexUpdate(chosenIndex,dst,uPad,uK,slotA,uK);
            if(lane==0u)chosenIndex=dst;
            barrier();
        }
    }
    if(lane==0u&&W[S]!=4u)endSource();
    memoryBarrierBuffer();barrier();
}
