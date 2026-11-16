// V4 scope comparison. Own scope identities must already be checked by finalize.
uniform uint uM3StateWords,uM3Header,uM3CombinedEventWord,uM3Rows,uM3CompareMode,uM3CompareResult;
// binding0/1 are the two combined state+event objects. Workspace contains the
// authority-event equality latch recorded before the consistency scope starts.
void m3SemanticFailure(uint word,uint expected,uint actual){
    if(W[0]!=0u)return;W[0]=16u;W[2]=uPass+1u;W[3]=word;W[4]=expected;W[5]=actual;
}
bool m3RepresentationWord(uint word){return word==uM3Header+76u||word==uM3Header+77u||word==uM3Header+78u||word==uM3Header+152u||word==uM3Header+153u;}
void m3CompareCombined(){
    uint word=gl_GlobalInvocationID.x;
    if(word<uM3StateWords&&!m3RepresentationWord(word)&&B[word]!=C[word])atomicMin(W[uM3CompareResult],word);
    uint eventWords=uM3Rows*64u;
    if(word<eventWords){uint mask=word%64u==52u?~(7u<<22u):0xffffffffu;uint expected=B[uM3CombinedEventWord+word]&mask,actual=C[uM3CombinedEventWord+word]&mask;if(expected!=actual)atomicMin(W[uM3CompareResult],uM3StateWords+word);}
}

// Called by the next existing PRIVATE_PROMOTE slot, on a single invocation,
// while selected/consistency remain bound. No additional readback or dispatch.
void m3FinishSemanticCompare(){
    uint mismatch=W[uM3CompareResult];if(mismatch==0xffffffffu)return;
    if(mismatch<uM3StateWords)m3SemanticFailure(mismatch,B[mismatch],C[mismatch]);
    else{uint word=mismatch-uM3StateWords;uint mask=word%64u==52u?~(7u<<22u):0xffffffffu;m3SemanticFailure(mismatch,B[uM3CombinedEventWord+word]&mask,C[uM3CombinedEventWord+word]&mask);}
}
