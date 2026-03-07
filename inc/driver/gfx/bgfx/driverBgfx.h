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
} driverBgfxVertex;
typedef struct driverBgfxTransform{
    float pos[3], rot[4], scale;
} driverBgfxTransform;
typedef struct driverBgfxShaderParams{
    float param1, param2, param3, param4;
} driverBgfxShaderParams;
typedef struct driverBgfxUniforms{
    bgfx_uniform_handle_t camPos, shaderParams1, shaderParams2;
} driverBgfxUniforms;
typedef struct driverBgfxMeshConfig{
    driverBgfxMeshType meshType;
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
typedef struct driverBgfxRenderItem{
    driverBgfxMesh mesh;
    driverBgfxTransform trans;
    bgfx_program_handle_t shader;
    driverBgfxShaderParams shaderParams1, shaderParams2;
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
    HWND hwnd;
    HDC hdc;
#endif
    uint32_t width, height;
    bgfx_shader_handle_t vertexShader, fragmentShader;
    bgfx_program_handle_t shaderProgram;
    bgfx_vertex_layout_t layout;
    driverBgfxUniforms uniforms;
} driverBgfx;

int driverBgfxOpen(void);
int driverBgfxClose(void);
int driverBgfxSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
