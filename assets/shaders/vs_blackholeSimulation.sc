$input a_position
$output v_pos
#include <bgfx_shader.sh>

void main(){
    // 로컬 정점 좌표를 그대로 프래그먼트 셰이더로 넘김 (구체 중심으로부터의 거리 계산용)
    v_pos = a_position;
    // 최종 화면 좌표 계산
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
}
