$input v_pos
#include <bgfx_shader.sh>

uniform vec4 u_camPos;
uniform vec4 u_shaderParams1; // x: type, y: mass, z: spin, w: time
//uniform vec4 u_shaderParams2; // x: disk_inner_radius, y: disk_outer_radius, z: disk_thickness, w: reserved

#define TYPE_STARFIELD 0.0
#define TYPE_BLACKHOLE 1.0
#define TYPE_ACCRETION_DISK 2.0
#define TYPE_NEUTRON_STAR 3.0

float starFieldHash(vec3 p){
    p = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}
vec3 starFieldGetStarColor(float h){ // 0.0 ~ 1.0 사이의 값 h를 입력받아 다양한 별 색상을 반환
    vec3 col1 = vec3(0.5, 0.7, 1.0); // 푸른 별
    vec3 col2 = vec3(1.0, 0.9, 0.8); // 노란 별
    vec3 col3 = vec3(1.0, 0.6, 0.5); // 붉은 별
    return mix(mix(col1, col2, h), col3, h * h);
}

void main(){
    float type = u_shaderParams1.x;
    float mass = u_shaderParams1.y;
    float time = u_shaderParams1.w;
    vec3 finalColor = vec3(0.0);
    float alpha = 1.0;

    if(type == TYPE_STARFIELD){ // 스타필드
        vec3 rd = normalize(v_pos); 
        // 별 생성
        float starNoise = starFieldHash(rd * 1000.0);
        float star = smoothstep(0.998, 1.0, starNoise);
        vec3 starColor = starFieldGetStarColor(starFieldHash(rd * 200.0));
        // 최종 합성
        finalColor = star * starColor;
    }else if(type == TYPE_BLACKHOLE){ // 블랙홀
        // [핵심] 질량(mass)에 따른 중력 렌즈 효과 계산 시작
        vec3 rayDir = normalize(v_pos);
        finalColor = vec3(0.1, 0.1, 0.3);
    }else if(type == TYPE_ACCRETION_DISK){ // 강착원반
        // [핵심] 레이 마칭 알고리즘 적용
        // u_shaderParams2의 데이터를 사용해 강착원반의 모양과 두께를 계산
        //float innerR = u_shaderParams2.x;
        //float outerR = u_shaderParams2.y;
        
        // 이 안에서 레이 마칭 루프를 돌며 밀도와 빛을 누적
        // finalColor = raymarchDisk(v_pos, mass, time);
    }
    // 최종 합성
    gl_FragColor = vec4(finalColor, alpha);
}
