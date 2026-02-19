/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-18
 ******************************************************************************/
#include "driver/driverCommon.h"

#if APP_DRIVER_PLATFORM == DRIVER_PLATFORM_WIN32

static driverPlatformWin32 _driverPlatformWin32 = {
    .objState = objStateClosed,
};

static LRESULT CALLBACK _driverPlatformWin32WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){ //logDebug("_driverPlatformWin32WindowProc msg=0x%X", msg);
    switch(msg){
        case WM_CLOSE:
            asyncPush(asyncTypeAsync, appMainEventPlatformWin32DestroyWindow, 0, 0, 0 ,0);
            return 0;
        case WM_SIZE:
            asyncPush(asyncTypeAsync, appMainEventPlatformWin32ResizeWindow, LOWORD(lParam), HIWORD(lParam), 0 ,0);
            return 0;
        case WM_PAINT:
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps); // 윈도우가 내부적으로 Validate를 수행함
            EndPaint(hwnd, &ps); // 윈도우에게 그리기가 끝났음을 공식 통보
            return 0;
        case WM_MOVE:
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
static void _driverPlatformWin32TimerHandler(void){ //logDebug("_driverPlatformWin32TimerHandler");
    //
}
int driverPlatformWin32Close(void){
    int result = retOk;
    if(_driverPlatformWin32.objState >= objStateOpening){
        osalMutexLock(&_driverPlatformWin32.objMutex, -1);
        _driverPlatformWin32.objState = objStateClosing;
        //
        _driverPlatformWin32.objState = objStateClosed;
closeExit:
        osalMutexUnlock(&_driverPlatformWin32.objMutex);
    }
    return result;
}
int driverPlatformWin32Open(void){
    int result = retOk;
    osalMutexOpen(&_driverPlatformWin32.objMutex);
    osalMutexLock(&_driverPlatformWin32.objMutex, -1);
    _driverPlatformWin32.objState = objStateOpening;
    //
    // Enroll Window Clss
    WNDCLASSEXW wndClass = {
        .cbSize = sizeof(WNDCLASSEXW), 
        .lpfnWndProc = _driverPlatformWin32WindowProc, 
        .hInstance = GetModuleHandle(NULL), 
        .lpszClassName = DRIVER_PLATFORM_WIN32_WINDOW_CLASS_NAME,
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
    };
    if(!RegisterClassExW(&wndClass)){ logError("RegisterClassExW fail");
        result = retFail;
        goto openExit;
    }
    //
    _driverPlatformWin32.objState = objStateOpened;
openExit:
    osalMutexUnlock(&_driverPlatformWin32.objMutex);
    return result;
}
int driverPlatformWin32Sync(uint16_t sync, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4){
    if(_driverPlatformWin32.objState < objStateOpened){ logError("objState(%d) < objStateOpened", _driverPlatformWin32.objState); return retFail; }
    int result = retOk;
    osalMutexLock(&_driverPlatformWin32.objMutex, -1);
    switch(sync){
        case driverPlatformWin32SyncTimer: //logDebug("driverPlatformWin32SyncTimer");
            _driverPlatformWin32TimerHandler();
            break;
        case driverPlatformWin32SyncCreateWindow:{ //logDebug("driverPlatformWin32SyncCreateWindow");
            if(!arg1 || !arg2 || !arg3){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            const char* titleUtf8 = (const char*)arg1;
            RECT rect = {0, 0, (LONG)arg2, (LONG)arg3};
            AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
            int width  = rect.right - rect.left;
            int height = rect.bottom - rect.top;
            wchar_t wideTitle[256];
            MultiByteToWideChar(CP_UTF8, 0, titleUtf8, -1, wideTitle, 256);
            _driverPlatformWin32.hwnd = CreateWindowExW(0, DRIVER_PLATFORM_WIN32_WINDOW_CLASS_NAME, wideTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, GetModuleHandle(NULL), NULL);
            if(!_driverPlatformWin32.hwnd){ logError("CreateWindowExW fail");
                result = retFail;
                goto syncExit;
            }
            ShowWindow(_driverPlatformWin32.hwnd, SW_SHOW);
            UpdateWindow(_driverPlatformWin32.hwnd);
            _driverPlatformWin32.hdc = GetDC(_driverPlatformWin32.hwnd);
            _driverPlatformWin32.running = 1;
            break;
        }
        case driverPlatformWin32SyncResizeWindow:
            if(!arg1 || !arg2){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            _driverPlatformWin32.width = (int)arg1;
            _driverPlatformWin32.height = (int)arg2;
            break;
        case driverPlatformWin32SyncDestroyWindow:
            if(_driverPlatformWin32.hwnd){
                ReleaseDC(_driverPlatformWin32.hwnd , _driverPlatformWin32.hdc);
                DestroyWindow(_driverPlatformWin32.hwnd);
                _driverPlatformWin32.hwnd = NULL;
                _driverPlatformWin32.hdc  = NULL;
            }
            break;
        case driverPlatformWin32SyncGetNativeHandle:
            if(!arg1 || !arg2){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            *(HWND*)arg1 = _driverPlatformWin32.hwnd;
            *(HDC*)arg2 = _driverPlatformWin32.hdc;
            break;
        case driverPlatformWin32SyncGetClientSize:
            if(!arg1 || !arg2){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            *(int*)arg1 = _driverPlatformWin32.width;
            *(int*)arg2 = _driverPlatformWin32.height;
            break;
    }
syncExit:
    osalMutexUnlock(&_driverPlatformWin32.objMutex);
    return result;
}
#endif