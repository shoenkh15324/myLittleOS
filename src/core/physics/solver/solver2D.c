/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-17
 ******************************************************************************/
#include "core/physics/solver/solver2D.h"
#include "core/physics/body/body2D.h"
#include "core/feature/log.h"
#include <math.h>

static float _slover2dClamp(float value, float max, float min){
    if(value > max) return max;
    if(value < min) return min;
    return value;
}
void solver2dCollisionSolve(contact2d** contacts, int contactCount){
    if(!contacts || contactCount <= 0){ logError("Invalid Params"); return; }
    for(int i = 0; i < contactCount; i++){
        contact2d* c = contacts[i];
        if(!c || !c->bodyA || !c->bodyB) continue;
        body2d* bodyA = c->bodyA;
        body2d* bodyB = c->bodyB;
        if(bodyA->isStatic && bodyB->isStatic) continue;
        vector2d relVelocity = vector2dSub(bodyB->velocity, bodyA->velocity); // 상대 속도
        float velAlongNormal = vector2dDot(relVelocity, c->normal); // 충돌 축 속도 (법선 방향)
        if(velAlongNormal > 0) continue;
        float invMassSum = bodyA->invMass + bodyB->invMass; // 충격량 [J]
        if(invMassSum == 0) continue;
        float restitution = 0.5f; // 반발 계수, 0 = 완전 비탄성, 1 = 완전 탄성
        float j = -(1 + restitution) * velAlongNormal / invMassSum;
        vector2d impulse = vector2dScale(c->normal, j);
        // 속도에 적용
        if(!bodyA->isStatic){ bodyA->velocity = vector2dSub(bodyA->velocity, vector2dScale(impulse, bodyA->invMass)); }
        if(!bodyB->isStatic){ bodyB->velocity = vector2dAdd(bodyB->velocity, vector2dScale(impulse, bodyB->invMass)); }
        // 침투 보정
        float percent = 0.2f; // 보정 비율
        float slop = 0.01f; // 허용 침투
        float correctionMagnitude = _slover2dClamp(c->penetration - slop, 0.0f, c->penetration) / invMassSum * percent;
        vector2d correction = vector2dScale(c->normal, correctionMagnitude);
        if(!bodyA->isStatic){ bodyA->position = vector2dSub(bodyA->position, vector2dScale(correction, bodyA->invMass)); }
        if(!bodyB->isStatic){ bodyB->position = vector2dAdd(bodyB->position, vector2dScale(correction, bodyB->invMass)); }
    }
}
