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
};
enum{
    driverBgfxStateXXX = objStateBegin,
};

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
    bgfx_vertex_buffer_handle_t vertexBufferHandle;
    bgfx_index_buffer_handle_t indexBufferHandle;
} driverBgfx;

int driverBgfxOpen(void);
int driverBgfxClose(void);
int driverBgfxSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
