/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-21
 ******************************************************************************/
#include "driver/driverCommon.h"
#if APP_DRIVER_GFX == DRIVER_GFX_BGFX
#include "par/par_shapes.h"

static driverBgfx _driverBgfx = {
    .objState = objStateClosed,
};

static int _driverBgfxCreateMesh(driverBgfxMeshConfig meshConfig, driverBgfxMesh* outputMesh){
    if(!outputMesh){ logError("Invalid Param"); return retFail; }
    par_shapes_mesh* parMesh = NULL;
    switch(meshConfig.meshType){
        case driverBgfxMeshTypeSphere:
            parMesh = par_shapes_create_subdivided_sphere(meshConfig.segment);
            break;
        case driverBgfxMeshTypeDisk:
            parMesh = par_shapes_create_disk(meshConfig.disk.radius, meshConfig.segment, (float[]){0,0,0}, (float[]){0,0,1});
            break;
        default: logError("Unsupported mesh type: %d", meshConfig.meshType);
            return retFail;
    }
    if(!parMesh){ logError("par_shapes_mesh creation failed"); return retFail; }
    outputMesh->numVertices = (uint32_t)parMesh->npoints;
    outputMesh->numIndices = (uint32_t)parMesh->ntriangles * 3;
    driverBgfxVertex* vertices;
    if(osalMalloc((void**)&vertices, sizeof(driverBgfxVertex) * outputMesh->numVertices)){ logError("osalMalloc fail"); 
        par_shapes_free_mesh(parMesh);
        return retFail;
    }
    for(uint32_t i = 0; i < outputMesh->numVertices; i++){
        vertices[i].x = parMesh->points[i * 3 + 0];
        vertices[i].y = parMesh->points[i * 3 + 1];
        vertices[i].z = parMesh->points[i * 3 + 2];
        vertices[i].abgr = meshConfig.abgr;
    }
    outputMesh->vbh = bgfx_create_vertex_buffer(bgfx_copy(vertices, sizeof(driverBgfxVertex) * outputMesh->numVertices), &_driverBgfx.layout, BGFX_BUFFER_NONE);
    outputMesh->ibh = bgfx_create_index_buffer(bgfx_copy(parMesh->triangles, sizeof(uint16_t) * outputMesh->numIndices), BGFX_BUFFER_NONE);
    if(osalFree(vertices)){ logError("osalFree fail"); return retFail; }
    par_shapes_free_mesh(parMesh);
    if(outputMesh->vbh.idx == 0xffff || outputMesh->ibh.idx == 0xffff){ logError("bgfx buffer creation failed"); return retFail; }
    return retOk;
}
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
    bgfx_reset(_driverBgfx.width, _driverBgfx.height, BGFX_RESET_VSYNC, BGFX_TEXTURE_FORMAT_COUNT);
    // 기본 상태 설정
    bgfx_set_debug(BGFX_DEBUG_TEXT);
    bgfx_set_view_rect(0, 0, 0, (uint16_t)_driverBgfx.width, (uint16_t)_driverBgfx.height);
    bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
    logInfo("driverBgfx opened (Size: %dx%d)", _driverBgfx.width, _driverBgfx.height);
    // 정점 레이아웃 (Vertex Layout) 정의
    bgfx_vertex_layout_begin(&_driverBgfx.layout, bgfx_get_renderer_type());
    bgfx_vertex_layout_add(&_driverBgfx.layout, BGFX_ATTRIB_POSITION, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&_driverBgfx.layout, BGFX_ATTRIB_COLOR0, 4, BGFX_ATTRIB_TYPE_UINT8, true, false);
    bgfx_vertex_layout_end(&_driverBgfx.layout);
    // Shader 로드
    _driverBgfx.vertexShader = _driverBgfxLoadShager("assets/shaders/glsl/vs_cubes.bin");
    _driverBgfx.fragmentShader = _driverBgfxLoadShager("assets/shaders/glsl/fs_cubes.bin");
    _driverBgfx.shaderProgram = bgfx_create_program(_driverBgfx.vertexShader, _driverBgfx.fragmentShader, true);
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
            if(!arg1){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            bgfx_set_view_rect(0, 0, 0, (uint16_t)_driverBgfx.width, (uint16_t)_driverBgfx.height);
            bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
            bgfx_touch(0);
            // 카메라 및 뷰 설정
            bgfxMat4 view, proj;
            bgfxMatViewLookat(&view, (bgfxVec3){0.0f, 20.0f, -35.0f}, (bgfxVec3){0.0f, 0.0f, 0.0f}, (bgfxVec3){0.0f, 1.0f, 0.0f});
            bgfxMathProjPerspective(&proj, 60.0f, (float) _driverBgfx.width / (float)_driverBgfx.height, 0.1f, 100.0f);
            bgfx_set_view_transform(0, view.m, proj.m);
            // 엔티티 순회 랜더링
            uint8_t* pEntityList = (uint8_t*)arg1;
            uint32_t entityCount = (uint32_t)arg2;
            uint32_t stride = (uint32_t)arg3;
            uint32_t offset = (uint32_t)arg4;
            for(uint32_t i = 0; i < entityCount; i++){
                uint8_t* pCurrEntity = pEntityList + (i * stride) + offset;
                driverBgfxRenderInfo* renderInfo = (driverBgfxRenderInfo*)pCurrEntity;
                float* pPos = renderInfo->trans.pos;
                float* pRot = renderInfo->trans.rot;
                float scale = renderInfo->trans.scale;
                bgfx_set_vertex_buffer(0, renderInfo->mesh.vbh, 0, UINT16_MAX);
                bgfx_set_index_buffer(renderInfo->mesh.ibh, 0, renderInfo->mesh.numIndices);
                bgfxMat4 mtx;
                bgfxMathTransform(&mtx, *(bgfxVec3*)pPos, *(bgfxQuat*)pRot, scale);
                bgfx_set_transform(mtx.m, 1);
                bgfx_set_vertex_buffer(0, renderInfo->mesh.vbh, 0, UINT16_MAX);
                bgfx_set_index_buffer(renderInfo->mesh.ibh, 0, renderInfo->mesh.numIndices);
                bgfx_program_handle_t program = (renderInfo->shader.idx != 0xffff) ? renderInfo->shader : _driverBgfx.shaderProgram;
                bgfx_set_state(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_MSAA, 0);
                bgfx_submit(0, program, 0, BGFX_DISCARD_ALL);
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
        case driverBgfxSyncCreateMesh:
            if(_driverBgfxCreateMesh(*(driverBgfxMeshConfig*)arg1, (driverBgfxRenderInfo*)arg2)){ logError("_driverBgfxCreateMesh Fail");
                result = retFail; goto syncExit;
            }
            break;
    }
syncExit:
    osalMutexUnlock(&_driverBgfx.objMutex);
    return result;
}
#endif
