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

static par_shapes_mesh* _driverBgfxCreateParMesh(driverBgfxMeshType type){
    switch(type){
        case driverBgfxMeshTypeSphere: 
            return par_shapes_create_subdivided_sphere(5);
        case driverBgfxMeshTypeQuad:
            return par_shapes_create_plane(1, 1);
        default: return NULL;
    }
}
static int _driverBgfxCreateMesh(driverBgfxRenderItem* item){
    if(!item){ logError("Invalid Param"); return retFail; }
    par_shapes_mesh* parMesh = _driverBgfxCreateParMesh(item->mesh.meshType);
    if(!parMesh){ logError("_driverBgfxCreateParMesh Fail"); return retFail; }
    item->mesh.numVertices = (uint32_t)parMesh->npoints;
    item->mesh.numIndices = (uint32_t)parMesh->ntriangles * 3;
    driverBgfxVertex* vertices;
    if(osalMalloc((void**)&vertices, (sizeof(driverBgfxVertex) * item->mesh.numVertices))){ logError("osalMalloc fail"); 
        par_shapes_free_mesh(parMesh);
        return retFail;
    }
    uint16_t* indices;
    if(osalMalloc((void**)&indices, (sizeof(uint16_t) * item->mesh.numIndices))){ logError("osalMalloc fail"); 
        par_shapes_free_mesh(parMesh);
        return retFail;
    }
    for(uint32_t i = 0; i < item->mesh.numVertices; i++){
        vertices[i].x = parMesh->points[i * 3];
        vertices[i].y = parMesh->points[i * 3 + 1];
        vertices[i].z = parMesh->points[i * 3 + 2];
    }
    for(uint32_t i = 0; i < item->mesh.numIndices; i++){
        indices[i] = (uint16_t)parMesh->triangles[i];
    }
    item->mesh.vbh = bgfx_create_vertex_buffer(bgfx_copy(vertices, (sizeof(*vertices) * item->mesh.numVertices)), &_driverBgfx.layout, BGFX_BUFFER_NONE);
    item->mesh.ibh = bgfx_create_index_buffer(bgfx_copy(indices, (sizeof(*indices) * item->mesh.numIndices)), BGFX_BUFFER_NONE);
    par_shapes_free_mesh(parMesh);
    osalFree(vertices); osalFree(indices);
    return (item->mesh.vbh.idx != 0xffff) ? retOk : retFail;
}
static int _driverBgfxDestroyMesh(driverBgfxRenderItem* item){
    if(!item){ logError("Invalid Param"); return retFail; }
    if(item->mesh.vbh.idx != UINT16_MAX){
        bgfx_destroy_vertex_buffer(item->mesh.vbh);
        item->mesh.vbh.idx = UINT16_MAX;
    }
    if(item->mesh.ibh.idx != UINT16_MAX){
        bgfx_destroy_index_buffer(item->mesh.ibh);
        item->mesh.ibh.idx = UINT16_MAX;
    }
    return retOk;
}
static bgfx_shader_handle_t _driverBgfxLoadShader(const char* filename){
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
static bgfx_program_handle_t _driverBgfxGetShaderProgram(driverBgfxShaderType type){
    switch(type){
        default:
            return _driverBgfx.shaders[0].shaderProgram; // default shader
    }
}
static void _driverBgfxApplyViewRects(uint16_t width, uint16_t height){
    for(uint16_t i = 0; i < driverBgfxPassTypeEnd; ++i){
        bgfx_set_view_rect(i, 0, 0, width, height);
    }
}
static int _driverBgfxGetPlatformInfo(void){
#if APP_OS == OS_WIN32
    if(driverPlatformWin32Sync(driverPlatformWin32SyncGetNativeHandle, (uintptr_t)&_driverBgfx.hwnd, (uintptr_t)&_driverBgfx.hdc, 0, 0)){ logError("driverPlatformWin32SyncGetNativeHandle fail"); 
        return retFail;
    }
    if(driverPlatformWin32Sync(driverPlatformWin32SyncGetClientSize, (uintptr_t)&_driverBgfx.width, (uintptr_t)&_driverBgfx.height, 0, 0)){ logError("driverPlatformWin32SyncGetNativeHandle fail");
        return retFail;
    }
#endif
    return retOk;
}
static int _driverBgfxInitRenderer(void){
    bgfx_init_t init;
    bgfx_init_ctor(&init);
    init.type = BGFX_RENDERER_TYPE_OPENGL;
#if APP_OS == OS_WIN32
    init.platformData.nwh = (void*)_driverBgfx.hwnd;
#endif
    init.resolution.width  = _driverBgfx.width;
    init.resolution.height = _driverBgfx.height;
    init.resolution.reset  = BGFX_RESET_VSYNC;
    if(!bgfx_init(&init)){ logError("bgfx_init fail"); 
        return retFail;
    }
    bgfx_reset(_driverBgfx.width, _driverBgfx.height, BGFX_RESET_VSYNC, BGFX_TEXTURE_FORMAT_COUNT);
    bgfx_set_debug(BGFX_DEBUG_TEXT);
    _driverBgfxApplyViewRects((uint16_t)init.resolution.width, (uint16_t)init.resolution.height);
    return retOk;
}
static int _driverBgfxCreateVertexLayout(void){
    bgfx_vertex_layout_begin(&_driverBgfx.layout, bgfx_get_renderer_type());
    bgfx_vertex_layout_add(&_driverBgfx.layout, BGFX_ATTRIB_POSITION, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_end(&_driverBgfx.layout);
    return retOk;
}
static int _driverBgfxSetShaders(void){
    _driverBgfx.shaders[0].vertexShader = _driverBgfxLoadShader("../assets/shaders/vs_blackholeSimulation.bin");
    _driverBgfx.shaders[0].fragmentShader = _driverBgfxLoadShader("../assets/shaders/fs_blackholeSimulation.bin");
    for(int i = 0; i < driverBgfxShaderTypeEnd; i++){
        if(_driverBgfx.shaders[i].vertexShader.idx == UINT16_MAX || _driverBgfx.shaders[i].fragmentShader.idx == UINT16_MAX){ logError("Shader load fail");
            return retFail;
        }
        _driverBgfx.shaders[i].shaderProgram = bgfx_create_program(_driverBgfx.shaders[i].vertexShader, _driverBgfx.shaders[i].fragmentShader, true);
        if(_driverBgfx.shaders[i].shaderProgram.idx == 0xFFFF){ logError("SHADER PROGRAM CREATION FAILED! Check shader files."); }
    }
    return retOk;
}
static int _driverBgfxSetUniforms(void){
    _driverBgfx.hShaderParams1 = bgfx_create_uniform("u_shaderParams1", BGFX_UNIFORM_TYPE_VEC4, 1);
    _driverBgfx.hCamPos = bgfx_create_uniform("u_camPos", BGFX_UNIFORM_TYPE_VEC4, 1);
    return retOk;
}
static int _driverBgfxInit(void){
    if(_driverBgfxGetPlatformInfo()){ logError("_driverBgfxGetPlatformInfo fail"); return retFail; }
    if(_driverBgfxInitRenderer()){ logError("_driverBgfxInitRenderer fail"); return retFail; }
    if(_driverBgfxCreateVertexLayout()){ logError("_driverBgfxCreateVertexLayout fail"); return retFail; }
    if(_driverBgfxSetShaders()){ logError("_driverBgfxSetShaders fail"); return retFail; }
    if(_driverBgfxSetUniforms()){ logError("_driverBgfxSetUniforms fail"); return retFail; }
    logInfo("driverBgfx opened (Size:%d X %d)", _driverBgfx.width, _driverBgfx.height);
    return retOk;
}
static int _driverBgfxClearUniforms(void){
    if(_driverBgfx.hShaderParams1.idx != UINT16_MAX){ bgfx_destroy_uniform(_driverBgfx.hShaderParams1); }
    return retOk;
}
static int _driverBgfxClearShaders(void){\
    for(int i = 0; i < driverBgfxShaderTypeEnd; i++){
        if(_driverBgfx.shaders[i].vertexShader.idx != UINT16_MAX){ bgfx_destroy_shader(_driverBgfx.shaders[i].vertexShader); }
        if(_driverBgfx.shaders[i].fragmentShader.idx != UINT16_MAX){ bgfx_destroy_shader(_driverBgfx.shaders[i].fragmentShader); }
    }
    return retOk;
}
static int _driverBgfxCleanup(void){
    if(_driverBgfxClearUniforms()){ logError("_driverBgfxClearUniforms fail");
        return retFail;
    }
    if(_driverBgfxClearShaders()){ logError("_driverBgfxClearShaders fail");
        return retFail;
    }
    bgfx_shutdown();
    return retOk;
}
int driverBgfxClose(void){
    int result = retOk;
    if(_driverBgfx.objState >= objStateOpening){
        osalMutexLock(&_driverBgfx.objMutex, -1);
        _driverBgfx.objState = objStateClosing;
        //
        if(_driverBgfxCleanup()){ logError("_driverBgfxCleanup fail");
            result = retFail; goto closeExit;
        }
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
        case driverBgfxSyncBeginFrame:
            _driverBgfxApplyViewRects((uint16_t)_driverBgfx.width, (uint16_t)_driverBgfx.height);
            break;
        case driverBgfxSyncSubmitItem:{
            if(!arg1){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            driverBgfxRenderItem* item = (driverBgfxRenderItem*)arg1;
            if(item->mesh.vbh.idx == UINT16_MAX || item->mesh.ibh.idx == UINT16_MAX){ logError("invalid buffers (VBH: %d, IBH: %d)", item->mesh.vbh.idx, item->mesh.ibh.idx);
                result = retFail; goto syncExit;
            }
            bgfx_set_vertex_buffer(0, item->mesh.vbh, 0, item->mesh.numVertices);
            bgfx_set_index_buffer(item->mesh.ibh, 0, item->mesh.numIndices);
            bgfxMat4 mtx;
            bgfxMathTransform(&mtx, *(bgfxVec3*)item->transform.pos, *(bgfxQuat*)item->transform.rot, item->transform.scale);
            bgfx_set_transform(mtx.m, 1);
            bgfx_set_uniform(_driverBgfx.hShaderParams1, &item->material.shaderParams, 1);
            uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;
            if(item->material.shaderParams.param1 == 0.0f){ // 배경(SkySphere)인 경우
                state |= BGFX_STATE_CULL_CW | BGFX_STATE_DEPTH_TEST_ALWAYS; // 시계 방향(뒷면) 그리기 / 깊이 테스트 유지 (배경이 젤 뒤에 가게)
            }else{
                state |= BGFX_STATE_CULL_CCW | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_WRITE_Z;
            }
            bgfx_set_state(state, 0);
            bgfx_program_handle_t shaderProgram = _driverBgfxGetShaderProgram(item->material.shaderType);
            bgfx_submit(_driverBgfx.currViewId, shaderProgram, 0, BGFX_DISCARD_ALL);
            break;
        }
        case driverBgfxSyncEndFrame:
            bgfx_frame(false);
            break;
        case driverBgfxSyncCreateMesh:
            if(!arg1){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            if(_driverBgfxCreateMesh((driverBgfxRenderItem*)arg1)){ logError("_driverBgfxCreateMesh Fail");
                result = retFail; goto syncExit;
            }
            break;
        case driverBgfxSyncDestroyMesh:
            if(!arg1){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            if(_driverBgfxDestroyMesh((driverBgfxRenderItem*)arg1)){ logError("_driverBgfxDestroyMesh Fail");
                result = retFail; goto syncExit;
            }
            break;
        case driverBgfxSyncUpdateViewport:
            if(!arg1 || !arg2){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            _driverBgfx.width = (uint32_t)arg1;
            _driverBgfx.height = (uint32_t)arg2;
            bgfx_reset((uint32_t)arg1, (uint32_t)arg2, BGFX_RESET_VSYNC, BGFX_TEXTURE_FORMAT_COUNT);
            _driverBgfxApplyViewRects((uint16_t)_driverBgfx.width, (uint16_t)_driverBgfx.height);
            break;
        case driverBgfxSyncSetPass:
            driverBgfxPassType pass = (driverBgfxPassType)arg1;
            switch(pass){
                case driverBgfxPassTypeBackground:
                    _driverBgfx.currViewId = 0;
                    bgfx_set_view_clear(_driverBgfx.currViewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
                    bgfx_set_view_mode(_driverBgfx.currViewId, BGFX_VIEW_MODE_SEQUENTIAL);
                    break;
                case driverBgfxPassTypeObject:
                    _driverBgfx.currViewId = 1;
                    bgfx_set_view_clear(_driverBgfx.currViewId, BGFX_CLEAR_DEPTH, 0, 1.0f, 0);
                    bgfx_set_view_mode(_driverBgfx.currViewId, BGFX_VIEW_MODE_DEPTH_ASCENDING);
                    break;
                case driverBgfxPassTypePostProcess:
                    _driverBgfx.currViewId = 2;
                    bgfx_set_view_clear(_driverBgfx.currViewId, BGFX_CLEAR_NONE, 0, 1.0f, 0);
                    bgfx_set_view_mode(_driverBgfx.currViewId, BGFX_VIEW_MODE_SEQUENTIAL);
                    break;
                default:
                    result = retFail; goto syncExit;
            }
            bgfx_touch(_driverBgfx.currViewId);
            break;
        case driverBgfxSyncSetCameraState:{
            if(!arg1){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            driverBgfxCameraState* camState = (driverBgfxCameraState*)arg1;
            bgfx_set_view_transform(_driverBgfx.currViewId, camState->viewMtx, camState->projMtx);
            bgfx_set_uniform(_driverBgfx.hCamPos, camState->camPos, 1);
            break;
        }
    }
syncExit:
    osalMutexUnlock(&_driverBgfx.objMutex);
    return result;
}
#endif
