/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-21
 ******************************************************************************/
#include "driver/driverCommon.h"
#if APP_DRIVER_GFX == DRIVER_GFX_BGFX
#include <bx/bx.h>
#include <bx/math.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <bgfx/c99/bgfx.h>

void bgfxMathIdentity(bgfxMat4* out){
    bx::mtxIdentity(out->m);
}
void bgfxMathTransform(bgfxMat4* out, bgfxVec3 pos, bgfxQuat rot, float scale){
    // scale -> rotate -> translate 순서로 행렬 생성
    float s[16], r[16], t[16], tmp[16];
    bx::mtxScale(s, scale, scale, scale);
    bx::mtxFromQuaternion(r, *(bx::Quaternion*)&rot);
    bx::mtxTranslate(t, pos.v[0], pos.v[1], pos.v[2]);
    bx::mtxMul(tmp, s, r);
    bx::mtxMul(out->m, tmp, t);
}
void bgfxMatViewLookat(bgfxMat4* out, bgfxVec3 eye, bgfxVec3 at, bgfxVec3 up){
    bx::mtxLookAt(out->m, *(bx::Vec3*)&eye, *(bx::Vec3*)&at, *(bx::Vec3*)&up);
}
void bgfxMathProjPerspective(bgfxMat4* out, float fovDeg, float aspect, float nearZ, float farZ){
    bx::mtxProj(out->m, fovDeg, aspect, nearZ, farZ, bgfx_get_caps()->homogeneousDepth);
}
void bgfxMathApplyTransform(const bgfxMat4* mtx){
    bgfx_set_transform(mtx->m, 1);
}
void bgfxMathApplyViewProj(uint16_t viewId, const bgfxMat4* view, const bgfxMat4* proj){
    bgfx_set_view_transform(viewId, view->m, proj->m);
}

#ifdef __cplusplus
}
#endif

#endif