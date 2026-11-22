#version 450
layout(set=0,binding=0,std430) readonly buffer M1Publication { uint words[]; };
layout(location=0) in vec2 uv;
layout(location=0) out vec4 color;

void main() {
    // FullM1 dry probe profile K=192, Kw=128, 128 columns, 256 height.
    const uint k = 192u;
    const uint header = 16u*k + 20u*128u + 16u*128u;
    color = vec4(0.05, 0.07, 0.10, 1);
    if (words[header] != 1347637831u || words[header+1u] != 2u ||
        words[header+3u] != 3235823838u) return;
    vec2 point = vec2(uv.x, 1.0-uv.y)*vec2(128, 256);
    uint live = min(words[header+34u], k);
    for (uint i = 0; i < live; ++i) {
        vec2 particle = vec2(uintBitsToFloat(words[8u*k+i]), uintBitsToFloat(words[9u*k+i]));
        if (length(point-particle) <= 1.5) {
            color = vec4(0,1,0,1);
            break;
        }
    }
}
