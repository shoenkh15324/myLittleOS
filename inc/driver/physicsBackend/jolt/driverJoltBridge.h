#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-21
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

typedef struct joltContext joltContext;

joltContext* joltInit(void);
int joltDeinit(joltContext* joltCtx);
unsigned int joltCreateFloor(joltContext* joltCtx, float px, float py, float pz, float w, float h, float d);
unsigned int joltCreateSphere(joltContext* joltCtx, float x, float y, float z, float radius, int isDynamic);
void joltStep(joltContext* joltCtx, float deltaTime, int collisionSteps);
void joltPrintBodyPosition(joltContext* joltCtx, unsigned int bodyId);
void joltGetBodyTransform(joltContext* joltCtx, unsigned int bodyId, float* outPos, float* outRot);




#ifdef __cplusplus
}
#endif