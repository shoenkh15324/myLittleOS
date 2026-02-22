#pragma
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-21
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

// 1. 행렬/벡터 데이터를 담는 범용 구조체 (bgfx와 호환되는 float[16])
typedef struct { float m[16]; } bgfxMat4;
typedef struct { float v[3]; }  bgfxVec3;
typedef struct { float q[4]; }  bgfxQuat;

// 2. 변환(Transform) 관련 추상화
void bgfxMathIdentity(bgfxMat4* out);
void bgfxMathTransform(bgfxMat4* out, bgfxVec3 pos, bgfxQuat rot, float scale);
// 3. 카메라/뷰 관련 추상화
void bgfxMatViewLookat(bgfxMat4* out, bgfxVec3 eye, bgfxVec3 at, bgfxVec3 up);
void bgfxMathProjPerspective(bgfxMat4* out, float fovDeg, float aspect, float nearZ, float farZ);
// 4. bgfx 즉시 적용 (편의 함수)
void bgfxMathApplyTransform(const bgfxMat4* mtx);
void bgfxMathApplyViewProj(uint16_t viewId, const bgfxMat4* view, const bgfxMat4* proj);


#ifdef __cplusplus
}
#endif