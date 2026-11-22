#version 450
layout(set=0,binding=0,std430) readonly buffer BootControl { uint control[]; };
layout(location=0) out vec4 color;
void main() {
    bool qualified = control[18687] == 0xc0de1301u && control[18439] == 0u;
    color = qualified ? vec4(0,1,0,1) : vec4(1,0,0,1);
}
