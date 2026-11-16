// T497 global W-only transfer, stable donor/receiver order and U64 budget.
// m3EmitLevel is the V4 semantic event writer supplied by the scope kernel.
void m3EmitLevel(uint donor,uint receiver,uint donorBefore,uint receiverBefore,uint amount,uvec2 budgetBefore,uvec2 budgetAfter);
bool m3WaterMatch(){
    uvec2 budget=uvec2(m3Get(M3_WATER_BUDGET),m3Get(M3_WATER_BUDGET+1u));
    for(uint donor=0u;donor<128u;++donor){
        for(uint receiver=0u;receiver<128u;++receiver){
            int errorD=int(m3Get(M3_WATER_ERROR+donor)),errorR=int(m3Get(M3_WATER_ERROR+receiver));
            if(errorD<=0||errorR>=0||all(equal(budget,uvec2(0u))))continue;
            uint donorWater=col(6u,donor),receiverWater=col(6u,receiver);
            uint capacity=m3Get(M3_CAP+receiver)-m3Get(M3_BASE_MATERIAL+receiver)-receiverWater;
            uint amount=min(uint(errorD),min(uint(-errorR),min(donorWater,capacity)));
            if(budget.y==0u)amount=min(amount,budget.x);
            if(amount==0u)continue;
            uvec2 after;if(!grsSub64(budget,uvec2(amount,0u),after))return false;
            // The event writer reserves credit BEFORE applying this patch.
            m3EmitLevel(donor,receiver,donorWater,receiverWater,amount,budget,after);
            if(W[0]!=0u)return false;
            C[uColumn+6u*128u+donor]=donorWater-amount;
            C[uColumn+6u*128u+receiver]=receiverWater+amount;
            C[uColumn+12u*128u+donor]=m3Get(M3_BASE_MATERIAL+donor)+donorWater-amount;
            C[uColumn+12u*128u+receiver]=m3Get(M3_BASE_MATERIAL+receiver)+receiverWater+amount;
            C[uColumn+13u*128u+donor]=C[uColumn+12u*128u+donor]+col(7u,donor);
            C[uColumn+13u*128u+receiver]=C[uColumn+12u*128u+receiver]+col(7u,receiver);
            m3Put(M3_WATER_ERROR+donor,uint(errorD-int(amount)));
            m3Put(M3_WATER_ERROR+receiver,uint(errorR+int(amount)));
            budget=after;
        }
    }
    m3Put(M3_WATER_BUDGET,budget.x);m3Put(M3_WATER_BUDGET+1u,budget.y);return true;
}
bool m3WaterProjectionAudit(){
    uvec2 actual=uvec2(0u),target=uvec2(0u);
    for(uint c=0u;c<128u;++c){
        uint water=col(6u,c),material=m3Get(M3_BASE_MATERIAL+c),cap=m3Get(M3_CAP+c);
        if(water>cap-material||m3Material(c)!=material||col(11u,c)!=material||col(12u,c)!=material+water||col(13u,c)!=material+water+col(7u,c))return false;
        m3Sum(actual,water);m3Sum(target,m3Get(M3_TARGET_WATER+c));
    }
    return all(equal(actual,target));
}
