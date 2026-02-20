#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-16
 ******************************************************************************/

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

#define SERVICE_RENDERING_ENABLE 1
 