/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-21
 ******************************************************************************/
#include "driver/driverCommon.h"
#if APP_DRIVER_GFX == DRIVER_GFX_BGFX

static driverBgfx _driverBgfx = {
    .objState = objStateClosed,
};
// 정점 구조체 (위치 + 색상)
typedef struct { float x, y, z; uint32_t abgr; } PosColorVertex;

// 박스 모양 데이터 (1x1x1 크기)
static PosColorVertex s_cubeVertices[] = {
    {-0.5f,  0.5f,  0.5f, 0xff0000ff }, {-0.5f, -0.5f,  0.5f, 0xff00ff00 },
    { 0.5f, -0.5f,  0.5f, 0xffff0000 }, { 0.5f,  0.5f,  0.5f, 0xffffffff },
    {-0.5f,  0.5f, -0.5f, 0xff00ffff }, {-0.5f, -0.5f, -0.5f, 0xffff00ff },
    { 0.5f, -0.5f, -0.5f, 0xffffff00 }, { 0.5f,  0.5f, -0.5f, 0xff000000 },
};

static const uint16_t s_cubeIndices[] = {
    0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4,
    1, 2, 6, 1, 6, 5, 2, 3, 7, 2, 7, 6, 3, 0, 4, 3, 4, 7,
};

static bgfx_shader_handle_t _driverBgfxLoadShager(const char* filename){
    FILE* file = fopen(filename, "rb");
    if(!file){ logError("Failed to open shader file: %s", filename);
        return (bgfx_shader_handle_t){0xffff};
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    const bgfx_memory_t* mem = bgfx_alloc(size);
    fread(mem->data, 1, size, file);
    fclose(file);
    return bgfx_create_shader(mem);
}
static int _driverBgfxInit(void){
    // 플랫폼 드라이버로부터 핸들 및 사이즈 정보 획득
#if APP_OS == OS_WIN32
    if(driverPlatformWin32Sync(driverPlatformWin32SyncGetNativeHandle, (uintptr_t)&_driverBgfx.hwnd, (uintptr_t)&_driverBgfx.hdc, 0, 0)){ logError("driverPlatformWin32SyncGetNativeHandle fail"); return retFail; }
    if(driverPlatformWin32Sync(driverPlatformWin32SyncGetClientSize, (uintptr_t)&_driverBgfx.width, (uintptr_t)&_driverBgfx.height, 0, 0)){ logError("driverPlatformWin32SyncGetNativeHandle fail"); return retFail; }
    logDebug("BGFX Init - HWND: %p, Width: %d, Height: %d", _driverBgfx.hwnd, _driverBgfx.width, _driverBgfx.height);
#endif
    // bgfx 초기화 설정
    bgfx_init_t init;
    bgfx_init_ctor(&init);
    init.type = BGFX_RENDERER_TYPE_OPENGL; // 최적의 렌더러 자동 선택
#if APP_OS == OS_WIN32
    init.platformData.nwh = (void*)_driverBgfx.hwnd;
#endif
    init.resolution.width = _driverBgfx.width;
    init.resolution.height = _driverBgfx.height;
    init.resolution.reset = BGFX_RESET_VSYNC;
    if(!bgfx_init(&init)){ logError("bgfx_init fail"); return retFail; }
    // 기본 상태 설정
    bgfx_set_debug(BGFX_DEBUG_TEXT);
    bgfx_set_view_rect(0, 0, 0, (uint16_t)_driverBgfx.width, (uint16_t)_driverBgfx.height);
    bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
    logInfo("driverBgfx opened (Size: %dx%d)", _driverBgfx.width, _driverBgfx.height);
    // 정점 레이아웃 (Vertex Layout) 정의
    bgfx_vertex_layout_t layout;
    bgfx_vertex_layout_begin(&layout, BGFX_RENDERER_TYPE_NOOP);
    bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_POSITION, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_COLOR0, 4, BGFX_ATTRIB_TYPE_UINT8, true, false);
    bgfx_vertex_layout_end(&layout);
    // Shader 로드
    _driverBgfx.vertexShader = _driverBgfxLoadShager("assets/shaders/glsl/vs_cubes.bin");
    _driverBgfx.fragmentShader = _driverBgfxLoadShager("assets/shaders/glsl/fs_cubes.bin");
    _driverBgfx.shaderProgram = bgfx_create_program(_driverBgfx.vertexShader, _driverBgfx.fragmentShader, true);
    _driverBgfx.vertexBufferHandle = bgfx_create_vertex_buffer(bgfx_make_ref(s_cubeVertices, sizeof(s_cubeVertices)), &layout, BGFX_BUFFER_NONE);
    _driverBgfx.indexBufferHandle = bgfx_create_index_buffer(bgfx_make_ref(s_cubeIndices, sizeof(s_cubeIndices)), BGFX_BUFFER_NONE);
    if(_driverBgfx.shaderProgram.idx == 0xffff){ logError("Failed to create shader program!"); }
    return retOk;
}
int driverBgfxClose(void){
    int result = retOk;
    if(_driverBgfx.objState >= objStateOpening){
        osalMutexLock(&_driverBgfx.objMutex, -1);
        _driverBgfx.objState = objStateClosing;
        //
        bgfx_shutdown();
        //
        _driverBgfx.objState = objStateClosed;
closeExit:
        osalMutexUnlock(&_driverBgfx.objMutex);
    }
    return result;
}
int driverBgfxOpen(void){
    int result = retOk;
    osalMutexOpen(&_driverBgfx.objMutex);
    osalMutexLock(&_driverBgfx.objMutex, -1);
    _driverBgfx.objState = objStateOpening;
    //
    //
    _driverBgfx.objState = objStateOpened;
openExit:
    osalMutexUnlock(&_driverBgfx.objMutex);
    return result;
}
int driverBgfxSync(uint16_t sync, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4){
    if(_driverBgfx.objState < objStateOpened){ logError("objState(%d) < objStateOpened", _driverBgfx.objState); return retFail; }
    int result = retOk;
    osalMutexLock(&_driverBgfx.objMutex, -1);
    switch(sync){
        case driverBgfxSyncInit:
            if(_driverBgfxInit()){ logError("_driverBgfxInit fail");
                result = retFail; goto syncExit;
            }
            break;
        case driverBgfxSyncRenderFrame:{
            bgfxVec3 pos = { *(float*)arg1, *((float*)arg1+1), *((float*)arg1+2) };
            bgfxQuat rot = { *(float*)arg2, *((float*)arg2+1), *((float*)arg2+2), *((float*)arg2+3) };
            bgfxMat4 view, proj, model;
            // 카메라를 (0, 2, -10)에 두고 (0, 0, 0)을 바라보게 설정
            bgfxMatViewLookat(&view, (bgfxVec3){0.0f, 2.0f, -10.0f}, (bgfxVec3){0.0f, 0.0f, 0.0f}, (bgfxVec3){0.0f, 1.0f, 0.0f});
            bgfxMathProjPerspective(&proj, 60.0f, (float)_driverBgfx.width/(float)_driverBgfx.height, 0.1f, 100.0f);
            
            // 0번 뷰에 카메라 행렬 적용
            bgfx_set_view_transform(0, view.m, proj.m);

            // 1. 바닥(Floor) 그리기
            bgfxMat4 floorMtx;
            bgfxVec3 floorPos = { 0.0f, 0.0f, 0.0f }; // Jolt에서 설정한 바닥 위치 (보통 0,0,0)
            bgfxQuat floorRot = { 0.0f, 0.0f, 0.0f, 1.0f }; 

            // 바닥은 아주 넓게 (Scale 100.0f)
            bgfxMathTransform(&floorMtx, floorPos, floorRot, 10.0f); 
            bgfx_set_transform(floorMtx.m, 1);

            bgfx_set_vertex_buffer(0, _driverBgfx.vertexBufferHandle, 0, 8);
            bgfx_set_index_buffer(_driverBgfx.indexBufferHandle, 0, 36);
            bgfx_set_state(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES, 0);
            bgfx_submit(0, _driverBgfx.shaderProgram, 0, BGFX_DISCARD_ALL);

            // 3. Jolt 물체 그리기 (주석 풀기)
            if(arg1 && arg2){ // 위치와 회전 데이터가 있을 때만
                bgfxVec3 cubePos = { *(float*)arg1, *((float*)arg1+1), *((float*)arg1+2) };
                bgfxQuat cubeRot = { *(float*)arg2, *((float*)arg2+1), *((float*)arg2+2), *((float*)arg2+3) };

                bgfxMat4 cubeMtx;
                bgfxMathTransform(&cubeMtx, cubePos, cubeRot, 1.0f); // 스케일 1.0
                bgfx_set_transform(cubeMtx.m, 1); // GPU에 이 행렬을 쓰라고 전달

                // 버퍼 세팅 및 제출
                bgfx_set_vertex_buffer(0, _driverBgfx.vertexBufferHandle, 0, 8);
                bgfx_set_index_buffer(_driverBgfx.indexBufferHandle, 0, 36);
                bgfx_submit(0, _driverBgfx.shaderProgram, 0, BGFX_DISCARD_ALL);
            }
            bgfx_frame(false);
            break;
        }
        case driverBgfxSyncUpdateViewport:
            if(!arg1 || !arg2){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            _driverBgfx.width = (uint32_t)arg1;
            _driverBgfx.height = (uint32_t)arg2;
            bgfx_reset((uint32_t)arg1, (uint32_t)arg2, BGFX_RESET_VSYNC, BGFX_TEXTURE_FORMAT_COUNT);
            break;
    }
syncExit:
    osalMutexUnlock(&_driverBgfx.objMutex);
    return result;
}
#endif
