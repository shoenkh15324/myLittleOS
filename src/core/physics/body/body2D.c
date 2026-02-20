/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-17
 ******************************************************************************/
#include "core/physics/body/body2D.h"
#include "core/physics/shape/shape2D.h"
#include "core/feature/log.h"
#include "core/feature/osal.h"

body2d* body2dCreate(vector2d position, float mass, shape2d* shape, int isStatic, uint32_t color){
    body2d* body = NULL;
    if(osalMalloc((void**)&body, sizeof(body2d))){ logError("osalMalloc fail");
        return NULL;
    }
    body->position = position;
    body->velocity = vector2dZero();
    body->force = vector2dZero();
    body->mass = mass;
    body->invMass = (isStatic || mass <= 0.0f) ? 0.0f : 1.0f / mass;
    body->shape = shape;
    if(shape){ shape->body = body; }
    body->isStatic = isStatic;
    body->color = color;
    return body;
}
void body2dDestroy(body2d* body){
    if(body){
        if(osalFree(body)){ logError("osalFree fail"); return; }
    }
}
void body2dApplyForce(body2d* body, vector2d force){
    if(!body || body->isStatic){ return; }
    body->force = vector2dAdd(body->force, force);
}
void body2dApplyImpulse(body2d* body, vector2d impulse){
    if(!body || body->isStatic){ return; }
    body->velocity  = vector2dAdd(body->velocity , vector2dScale(impulse, body->invMass));
}
void body2dIntegrationForces(body2d* body, float dt){
    if(!body || body->isStatic){ return; }
    vector2d acceleration = vector2dScale(body->force, body-> invMass);
    body->velocity = vector2dAdd(body->velocity, vector2dScale(acceleration, dt));
    body->force = vector2dZero();
}
void body2dIntegrationVelocity(body2d* body, float dt){
    if(!body || body->isStatic){ return; }
    body->position = vector2dAdd(body->position, vector2dScale(body->velocity, dt));
}
