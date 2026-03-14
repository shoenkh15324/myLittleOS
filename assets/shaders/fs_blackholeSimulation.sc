$input v_dir
#include <bgfx_shader.sh>

SAMPLER2D(s_hdr, 0);

uniform vec4 u_camPos;
uniform vec4 u_shaderParams1; // x: type, y: mass, z: spin, w: time
//uniform vec4 u_shaderParams2; // x: disk_inner_radius, y: disk_outer_radius, z: disk_thickness, w: reserved

#define TYPE_STARFIELD 0.0
#define TYPE_BLACKHOLE 1.0
#define TYPE_ACCRETION_DISK 2.0
#define TYPE_NEUTRON_STAR 3.0

void main(){
    float type = u_shaderParams1.x;
    float mass = u_shaderParams1.y;
    vec3 rd = normalize(v_dir);
    vec3 finalColor = vec3(0.0);

    if(type < 0.5){ // TYPE_STARFIELD
        float phi = atan(rd.z, rd.x);
        float theta = asin(rd.y);
        vec2 uv;
        uv.x = 0.5 + phi / (2.0 * 3.1415926535);
        uv.y = 0.5 - theta / 3.1415926535;
        finalColor = texture2D(s_hdr, uv).rgb;
    }else if(type > 0.5 && type < 1.5){ // TYPE_BLACKHOLE
        float mass = u_shaderParams1.y; // 중력 강도
        float dist = length(rd); 
        float angleToCenter = acos(dot(rd, vec3(0.0, 0.0, 1.0)));
        float bend = mass / (max(0.1, angleToCenter) * 10.0);
        vec3 warpedRd = normalize(rd + vec3(0.0, 0.0, -bend)); 
        float phi = atan(warpedRd.z, warpedRd.x);
        float theta = asin(warpedRd.y);
        vec2 uv = vec2(0.5 + phi / (2.0 * 3.14159), 0.5 - theta / 3.14159);
        finalColor = texture2D(s_hdr, uv).rgb;
        if(angleToCenter < (mass * 0.05)) finalColor = vec3(0.0);
    }
    gl_FragColor = vec4(finalColor,1.0);
}
