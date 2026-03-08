#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-21
 ******************************************************************************/
#include "core/feature/osal.h"
#include <bgfx/c99/bgfx.h>

enum{
    driverBgfxSyncInit = objSyncBegin,
    driverBgfxSyncBeginFrame,
    driverBgfxSyncSubmitItem,
    driverBgfxSyncEndFrame,
    driverBgfxSyncCreateMesh,
    driverBgfxSyncDestroyMesh,
    driverBgfxSyncUpdateViewport,
    driverBgfxSyncSetPass,
    driverBgfxSyncSetCameraState,
};
enum{
    driverBgfxStateXXX = objStateBegin,
};
typedef enum{
    driverBgfxPassTypeBackground,
    driverBgfxPassTypeObject,
    driverBgfxPassTypePostProcess,
    driverBgfxPassTypeEnd,
} driverBgfxPassType;
typedef enum{
    driverBgfxMeshTypeSphere,
    driverBgfxMeshTypeQuad,
} driverBgfxMeshType;
typedef enum{
    driverBgfxShaderTypeDefault,
    driverBgfxShaderTypeEnd,
} driverBgfxShaderType;

typedef struct driverBgfxVertex{
    float x, y, z;
} driverBgfxVertex;
typedef struct driverBgfxCameraState{
    float viewMtx[16], projMtx[16], camPos[4];
} driverBgfxCameraState;
typedef struct driverBgfxShaders{
    bgfx_shader_handle_t vertexShader, fragmentShader;
    bgfx_program_handle_t shaderProgram;
} driverBgfxShaders;
typedef struct driverBgfxShaderParams{
    float param1, param2, param3, param4;
} driverBgfxShaderParams;
typedef struct driverBgfxMesh{
    driverBgfxMeshType meshType;
    bgfx_vertex_buffer_handle_t vbh;
    bgfx_index_buffer_handle_t ibh;
    uint32_t numVertices;
    uint32_t numIndices;
} driverBgfxMesh;
typedef struct driverBgfxTransform{
    float pos[3], rot[4], scale;
} driverBgfxTransform;
typedef struct driverBgfxMaterial{
    driverBgfxShaderType shaderType;
    bgfx_program_handle_t shader;
    uint64_t state;
    driverBgfxShaderParams shaderParams;
} driverBgfxMaterial;
typedef struct driverBgfxRenderItem{
    driverBgfxMesh mesh;
    driverBgfxMaterial material;
    driverBgfxTransform transform;
} driverBgfxRenderItem;
typedef struct driverBgfx{
    objectState objState;
    osalMutex objMutex;
#if APP_OS == OS_WIN32
    HWND hwnd; HDC hdc;
#endif
    uint16_t currViewId;
    uint32_t width, height;
    bgfx_vertex_layout_t layout;
    bgfx_uniform_handle_t hShaderParams1, hCamPos;
    driverBgfxShaders shaders[driverBgfxShaderTypeEnd];
} driverBgfx;

int driverBgfxOpen(void);
int driverBgfxClose(void);
int driverBgfxSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
