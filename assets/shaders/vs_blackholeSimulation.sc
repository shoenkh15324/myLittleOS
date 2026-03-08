$input a_position
$output v_pos
#include <bgfx_shader.sh>

uniform vec4 u_camPos;

void main(){
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_pos = a_position - u_camPos.xyz;
}
