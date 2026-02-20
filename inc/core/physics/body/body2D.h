#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-17
 ******************************************************************************/
#include "core/physics/vector/vector2D.h"

typedef struct shape2d shape2d;

typedef struct body2d{
    vector2d position, velocity, force;
    float mass, invMass;
    shape2d* shape;
    uint32_t color;
    int isStatic; // If it's 1, it doesn't move
} body2d;

body2d* body2dCreate(vector2d, float, shape2d*, int, uint32_t);
void body2dDestroy(body2d*);
void body2dApplyForce(body2d*, vector2d);
void body2dApplyImpulse(body2d*, vector2d);
void body2dIntegrationForces(body2d*, float);
void body2dIntegrationVelocity(body2d*, float);
