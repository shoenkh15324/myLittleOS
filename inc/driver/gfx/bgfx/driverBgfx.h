#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-21
 ******************************************************************************/
#include "core/feature/osal.h"
#include <bgfx/c99/bgfx.h>

enum{
    driverBgfxSyncInit = objSyncBegin,
    driverBgfxSyncRenderFrame,
    driverBgfxSyncUpdateViewport,
    driverBgfxSyncCreateMesh,
};
enum{
    driverBgfxStateXXX = objStateBegin,
};
typedef enum{
    driverBgfxMeshTypeSphere,
    driverBgfxMeshTypeDisk,
} driverBgfxMeshType;

typedef struct driverBgfxVertex{
    float x, y, z;
    uint32_t abgr;
} driverBgfxVertex;
typedef struct driverBgfxTransform{
    float pos[3], rot[4], scale;
} driverBgfxTransform;
typedef struct driverBgfxMeshConfig{
    driverBgfxMeshType meshType;
    uint32_t abgr;
    int segment;
    union{
        struct{
            float radius;
        } sphere, disk;
        struct{
            float width, height, depth;
        } cube;
    };
} driverBgfxMeshConfig;
typedef struct driverBgfxMesh{
    bgfx_vertex_buffer_handle_t vbh;
    bgfx_index_buffer_handle_t ibh;
    uint32_t numVertices;
    uint32_t numIndices;
} driverBgfxMesh;
typedef struct driverBgfxRenderInfo{
    driverBgfxMesh mesh;
    driverBgfxTransform trans;
    bgfx_program_handle_t shader;
} driverBgfxRenderInfo;
typedef struct driverBgfx{
    objectState objState;
    osalMutex objMutex;
#if APP_OS == OS_WIN32
    HWND hwnd;
    HDC hdc;
#endif
    uint32_t width, height;
    bgfx_shader_handle_t vertexShader, fragmentShader;
    bgfx_program_handle_t shaderProgram;
    bgfx_vertex_layout_t layout;
} driverBgfx;

int driverBgfxOpen(void);
int driverBgfxClose(void);
int driverBgfxSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
