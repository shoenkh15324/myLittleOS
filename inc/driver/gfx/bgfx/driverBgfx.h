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
    driverBgfxSyncSetPass,
    driverBgfxSyncSubmitItem,
    driverBgfxSyncEndFrame,
    driverBgfxSyncCreateMesh,
    driverBgfxSyncUpdateViewport,
};
enum{
    driverBgfxStateXXX = objStateBegin,
};
typedef enum{
    driverBgfxPassTypeBackground,
    driverBgfxPassTypeObject,
    driverBgfxPassTypePostProcess,
} driverBgfxPassType;
typedef enum{
    driverBgfxMeshTypeSphere,
    driverBgfxMeshTypeQuad,
} driverBgfxMeshType;

typedef struct driverBgfxVertex{
    float x, y, z;
} driverBgfxVertex;
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
    bgfx_program_handle_t shader;
    uint64_t state;
    float uniforms[16];
} driverBgfxMaterial;
typedef struct driverBgfxRenderItem{
    driverBgfxMesh mesh;
    driverBgfxMaterial material;
    driverBgfxTransform transform;
} driverBgfxRenderItem;
typedef struct driverBgfxSceneContext{
    uint8_t* pItems;
    uint32_t itemCount, itemStride, itemOffset;
    float camPos[3], camFront[3], fov;
} driverBgfxSceneContext;
typedef struct driverBgfx{
    objectState objState;
    osalMutex objMutex;
#if APP_OS == OS_WIN32
    HWND hwnd; HDC hdc;
#endif
    uint16_t currViewId;
    uint32_t width, height;
    bgfx_shader_handle_t vertexShader, fragmentShader;
    bgfx_vertex_layout_t layout;
} driverBgfx;

int driverBgfxOpen(void);
int driverBgfxClose(void);
int driverBgfxSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
