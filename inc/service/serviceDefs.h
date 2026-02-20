#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-16
 ******************************************************************************/

#define COLOR_RED 0xFFFF0000
#define COLOR_GREEN 0xFF00FF00
#define COLOR_BLUE 0xFF0000FF
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_BLACK 0xFF000000

typedef struct drawCircle2D{
    float x, y, radius;
} drawCircle2D;
typedef struct drawRect2D{
    float x, y, width, height, rotation;
} drawRect2D;
typedef struct drawLine2D{
    float x1, y1, x2, y2, thickness;
} drawLine2D;

typedef enum{
    renderCmdTypeCircle,
    renderCmdTypeRect,
    renderCmdTypeLine,
} renderCmdType;

typedef struct renderCmd{
    renderCmdType type;
    uint32_t color;
    int layer;
    union{
        drawCircle2D circle;
        drawRect2D rect;
        drawLine2D line;
    } diagram;
} renderCmd;

#define SERVICE_RENDERING_2D 1
#define SERVICE_RENDERING_3D 2
 