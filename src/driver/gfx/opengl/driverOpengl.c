/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-18
 ******************************************************************************/
#include "driver/driverCommon.h"

#if APP_DRIVER_GFX == DRIVER_GFX_OPENGL
#include <GL/gl.h>
#include <math.h>
#include "core/physics/vector/vector2D.h"

static driverOpengl _driverOpengl = {
    .objState = objStateClosed,
    .currColor = {1.0f, 1.0f, 1.0f},
};

static GLenum _convertPrimitive(gfxPrimitiveType type){
    switch(type){
        case gfxPrimitiveTypePoints:
            return GL_POINTS;
        case gfxPrimitiveTypeLines:
            return GL_LINES;
        case gfxPrimitiveTypeTriangles:
            return GL_TRIANGLES;
        default:
            return GL_TRIANGLES;
    }
}
static void _driverOpenglUpdateViewport(int width, int height){
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);  // 좌상단 (0,0)
    glMatrixMode(GL_MODELVIEW);
}
static void _driverOpenglDrawPrimitive(gfxPrimitiveType type, const float* vertices, int vertexCount){
    if(vertices == NULL || vertexCount <= 0){ logError("Invliad Params"); return; }
    glColor3f(_driverOpengl.currColor[0], _driverOpengl.currColor[1], _driverOpengl.currColor[2]);
    GLenum mode = _convertPrimitive(type);
    glBegin(mode);
    for(int i = 0; i < vertexCount; ++i){
        float x = vertices[i * 2 + 0];
        float y = vertices[i * 2 + 1];
        glVertex2f(x, y);
    }
    glEnd();
}
static int _driverOpenglInit(void){
#if APP_OS == OS_WIN32
    if(driverPlatformWin32Sync(driverPlatformWin32SyncGetNativeHandle, (uintptr_t)&_driverOpengl.hwnd, (uintptr_t)&_driverOpengl.hdc, 0, 0)){ logError("driverPlatformWin32SyncGetNativeHandle fail");
        return retFail;
    }
    PIXELFORMATDESCRIPTOR pfd = {
        .nSize = sizeof(pfd),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 32,
        .cDepthBits = 24,
        .iLayerType = PFD_MAIN_PLANE,
    };
    int pf = ChoosePixelFormat(_driverOpengl.hdc, &pfd);
    if(!pf || !SetPixelFormat(_driverOpengl.hdc, pf, &pfd)){ logError("SetPixelFormat fail");
        return retFail;
    }
    _driverOpengl.glrc = wglCreateContext(_driverOpengl.hdc);
    if(!_driverOpengl.glrc){ logError("wglCreateContext fail");
        return retFail;
    }
    wglMakeCurrent(_driverOpengl.hdc, _driverOpengl.glrc);
    RECT rect;
    GetClientRect(_driverOpengl.hwnd, &rect);
    int width  = rect.right  - rect.left;
    int height = rect.bottom - rect.top;
    _driverOpenglUpdateViewport(width, height);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);  // 2D이므로 depth 끄기
#endif
    return retOk;
}
static int _driverOpenglBeginFrame(void){
    glClear(GL_COLOR_BUFFER_BIT);
    return retOk;
}
static int _driverOpenglEndFrame(void){
#if APP_OS == OS_WIN32
    SwapBuffers(_driverOpengl.hdc);
#endif
    return retOk;
}
static int _driverOpenglDeinit(void){
#if APP_OS == OS_WIN32
    if(_driverOpengl.glrc){
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(_driverOpengl.glrc);
        _driverOpengl.glrc = NULL;
    }
    if(_driverOpengl.hdc && _driverOpengl.hwnd){
        ReleaseDC(_driverOpengl.hwnd, _driverOpengl.hdc);
        _driverOpengl.hdc = NULL;
    }
#endif
    return retOk;
}
int driverOpenglClose(void){
    int result = retOk;
    if(_driverOpengl.objState >= objStateOpening){
        osalMutexLock(&_driverOpengl.objMutex, -1);
        _driverOpengl.objState = objStateClosing;
        //
        //
        _driverOpengl.objState = objStateClosed;
closeExit:
        osalMutexUnlock(&_driverOpengl.objMutex);
    }
    return result;
}
int driverOpenglOpen(void){
    int result = retOk;
    osalMutexOpen(&_driverOpengl.objMutex);
    osalMutexLock(&_driverOpengl.objMutex, -1);
    _driverOpengl.objState = objStateOpening;
    //
    //
    _driverOpengl.objState = objStateOpened;
openExit:
    osalMutexUnlock(&_driverOpengl.objMutex);
    return result;
}
int driverOpenglSync(uint16_t sync, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4){
    if(_driverOpengl.objState < objStateOpened){ logError("objState(%d) < objStateOpened", _driverOpengl.objState); return retFail; }
    int result = retOk;
    osalMutexLock(&_driverOpengl.objMutex, -1);
    switch(sync){
        case driverOpenglSyncTimer: //logDebug("driverOpenglSyncTimer");
            break;
        case driverOpenglSyncInit:
            if(_driverOpenglInit()){ logError("_driverOpenglInit fail");
                result = retFail; goto syncExit;
            }
            break;
        case driverOpenglSyncDeinit:
            if(_driverOpenglDeinit()){ logError("_driverOpenglDeinit fail");
                result = retFail; goto syncExit;
            }
            break;
        case driverOpenglSyncBeginFrame:
            _driverOpenglBeginFrame();
            break;
        case driverOpenglSyncEndFrame:
            _driverOpenglEndFrame();
            break;
        case driverOpenglSyncDrawPrimitive:
            if(!arg2 || !arg3){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            _driverOpenglDrawPrimitive((gfxPrimitiveType)arg1, (const float*)arg2, (int)arg3);
            break;
        case driverOpenglSyncClear:
            glClear(GL_COLOR_BUFFER_BIT);
            break;
        case driverOpenglSyncSetClearColor:
            glClearColor((float)arg1/255.0f, (float)arg2/255.0f, (float)arg3/255.0f, 1.0f);
            break;
        case driverOpenglSyncSetColor:
            _driverOpengl.currColor[0] = (float)arg1/255.0f;
            _driverOpengl.currColor[1] = (float)arg2/255.0f;
            _driverOpengl.currColor[2] = (float)arg3/255.0f;
            glColor3f(_driverOpengl.currColor[0], _driverOpengl.currColor[1], _driverOpengl.currColor[2]);
            break;
        case driverOpenglSyncUpdateViewport:
            if(!arg1 || !arg2){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            _driverOpenglUpdateViewport(arg1, arg2);
            break;
    }
syncExit:
    osalMutexUnlock(&_driverOpengl.objMutex);
    return result;
}
#endif
