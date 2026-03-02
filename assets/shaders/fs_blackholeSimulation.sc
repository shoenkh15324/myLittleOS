$input v_pos
#include <bgfx_shader.sh>

uniform vec4 surfaceColor; 
uniform vec4 surfaceParams; // x: emission, y: opacity, z: metallic, w: roughness

void main(){
    vec3 base = surfaceColor.rgb;
    float emission = surfaceParams.x;
    float opacity = surfaceParams.y;
    // 단순 출력 + 발광 효과 (에너지가 넘치는 강착 원반 표현용)
    vec3 finalColor = base + (base * emission);
    gl_FragColor = vec4(finalColor, surfaceColor.a * opacity);
}
