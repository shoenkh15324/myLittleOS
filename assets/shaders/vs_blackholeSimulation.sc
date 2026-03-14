$input a_position
$output v_dir
#include <bgfx_shader.sh>

void main(){
    vec4 pos = vec4(a_position, 1.0);
    gl_Position = mul(u_modelViewProj, pos);
    v_dir = a_position;
}
