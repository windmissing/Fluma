// Deterministic polynomial on [-pi/2, pi/2]. The G16 pose lies in [-pi, pi].
// Explicit non-contraction keeps the cancellation-sensitive gap reproducible.
vec2 g16CosSin(float angle) {
    const float pi=3.14159265358979323846;
    precise float x=angle;
    float cosineSign=1.;
    if (x>pi*.5) { x=pi-x; cosineSign=-1.; }
    else if (x< -pi*.5) { x=-pi-x; cosineSign=-1.; }
    precise float x2=x*x;
    precise float c=1.+x2*(-.5+x2*(.0416666666666667+x2*(-.00138888888888889+
        x2*(.0000248015873015873+x2*(-.000000275573192239859+x2*.00000000208767569878681)))));
    precise float s=x*(1.+x2*(-.166666666666667+x2*(.00833333333333333+
        x2*(-.000198412698412698+x2*(.00000275573192239859+
        x2*(-.0000000250521083854417+x2*.000000000160590438368216))))));
    return vec2(cosineSign*c,s);
}
