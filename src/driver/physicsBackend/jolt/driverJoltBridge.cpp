/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-21
 ******************************************************************************/
#include "driver/driverCommon.h"
#if APP_DRIVER_PHYSICS_BACKEND == DRIVER_PHYSICS_BACKEND_JOLT
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

using namespace std;
using namespace JPH;

namespace Layers{
	static constexpr ObjectLayer NON_MOVING = 0;
	static constexpr ObjectLayer MOVING = 1;
	static constexpr ObjectLayer NUM_LAYERS = 2;
};
namespace BroadPhaseLayers{
	static constexpr BroadPhaseLayer NON_MOVING(0);
	static constexpr BroadPhaseLayer MOVING(1);
	static constexpr uint NUM_LAYERS(2);
};

class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface{
public:
    BPLayerInterfaceImpl(){
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	}
	virtual uint GetNumBroadPhaseLayers() const override{
		return BroadPhaseLayers::NUM_LAYERS;
	}
	virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override{
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override{
		switch ((BroadPhaseLayer::Type)inLayer){
            case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
            case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING: return "MOVING";
            default: JPH_ASSERT(false); return "INVALID";
		}
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED
private:
	BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};
class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter{
public:
	virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override{
		switch (inLayer1){
            case Layers::NON_MOVING:
                return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
		}
	}
};
class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter{
public:
	virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override{
		switch (inObject1){
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING; // Non moving only collides with moving
            case Layers::MOVING:
                return true; // Moving collides with everything
            default:
                JPH_ASSERT(false);
                return false;
		}
	}
};

#ifdef __cplusplus
extern "C" {
#endif

#include "core/feature/log.h"

struct joltContext{
    TempAllocatorImpl* tempAllocator;
    JobSystemThreadPool* jobSystem;
    PhysicsSystem* physicsSystem;
    BPLayerInterfaceImpl bpLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadphaseLayerFilter;
    ObjectLayerPairFilterImpl objectVsObjectLayerFilter;
};

joltContext* joltInit(void){
    RegisterDefaultAllocator();
    Factory::sInstance = new Factory();
    RegisterTypes();
    joltContext* joltCtx = new joltContext();
    joltCtx->tempAllocator = new TempAllocatorImpl(DRIVER_PHYSICS_BACKEND_JOLT_TEMP_ALLOCATOR_SIZE);
    joltCtx->jobSystem = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - APP_THREAD_MAX_COUNT);
    joltCtx->physicsSystem = new PhysicsSystem();
    joltCtx->physicsSystem->Init(
        DRIVER_PHYSICS_BACKEND_JOLT_MAX_BODIES, 
        DRIVER_PHYSICS_BACKEND_JOLT_NUM_BODY_MUTEXES, 
        DRIVER_PHYSICS_BACKEND_JOLT_NUM_BODY_PAIRS,
        DRIVER_PHYSICS_BACKEND_JOLT_MAX_CONTACT_CONSTRAINTS,
        joltCtx->bpLayerInterface,
        joltCtx->objectVsBroadphaseLayerFilter,
        joltCtx->objectVsObjectLayerFilter
    );
    
    return joltCtx;
}
int joltDeinit(joltContext* joltCtx){
    if(!joltCtx){ logError("Invalid Params"); return retFail; }
    delete joltCtx->physicsSystem;
    delete joltCtx->jobSystem;
    delete joltCtx->tempAllocator;
    if(Factory::sInstance){
        delete Factory::sInstance;
        Factory::sInstance = nullptr;
    }
    delete joltCtx;
    return retOk;
}
unsigned int joltCreateFloor(joltContext* joltCtx, float px, float py, float pz, float w, float h, float d){
    if(!joltCtx || w <= 0 || h <= 0 || d <= 0){ logError("Invalid Params"); return 0; }
    BodyInterface &bodyInterface = joltCtx->physicsSystem->GetBodyInterface();
    BoxShapeSettings floorShapeSettings(Vec3(w * 0.5f, h * 0.5f, d * 0.5f));
    BodyCreationSettings floorSettings(
        floorShapeSettings.Create().Get(),
        RVec3(px, py, pz),
        Quat::sIdentity(),
        EMotionType::Static,
        Layers::NON_MOVING
    );
    Body* floor = bodyInterface.CreateBody(floorSettings);
    bodyInterface.AddBody(floor->GetID(), EActivation::DontActivate);
    return floor->GetID().GetIndexAndSequenceNumber();
}
unsigned int joltCreateSphere(joltContext* joltCtx, float x, float y, float z, float radius, int isDynamic){
    if(!joltCtx){ logError("Invalid Params"); return 0; }
    BodyInterface &bodyInterface = joltCtx->physicsSystem->GetBodyInterface();
    SphereShapeSettings shapeSettings(radius);
    ShapeSettings::ShapeResult result = shapeSettings.Create();
    if(result.HasError()){ logError("Shape Creation Failed: %s", result.GetError().c_str());
        return retFail;
    }
    EMotionType motionType = isDynamic ? EMotionType::Dynamic : EMotionType::Static;
    ObjectLayer layer = isDynamic ? Layers::MOVING : Layers::NON_MOVING;
    BodyCreationSettings sphereSettings(result.Get(), RVec3(x, y, z), Quat::sIdentity(), motionType, layer);
    BodyID sphereId = bodyInterface.CreateAndAddBody(sphereSettings, EActivation::Activate);
    if(isDynamic){ bodyInterface.SetLinearVelocity(sphereId, Vec3(0.0f, -0.1f, 0.0f)); }
    return (unsigned int)sphereId.GetIndexAndSequenceNumber();
}
void joltStep(joltContext* joltCtx, float deltaTime, int collisionSteps){
    if(!joltCtx){ logError("Invalid Params"); return; }
    EPhysicsUpdateError error = joltCtx->physicsSystem->Update(deltaTime, collisionSteps, joltCtx->tempAllocator, joltCtx->jobSystem);
    if(error != EPhysicsUpdateError::None){ logError("Jolt Update Error: %d", (int)error); }
}
void joltPrintBodyPosition(joltContext* joltCtx, unsigned int bodyId){
    if(!joltCtx){ logError("Invalid Params"); return; }
    BodyID id(bodyId);
    RVec3 pos = joltCtx->physicsSystem->GetBodyInterface().GetPosition(id);
    logDebug("Body ID [%u] Position: x=%.2f, y=%.2f, z=%.2f", bodyId, (float)pos.GetX(), (float)pos.GetY(), (float)pos.GetZ());
}
void joltGetBodyTransform(joltContext* joltCtx, unsigned int bodyId, float* outPos, float* outRot){
    if(!joltCtx){ logError("Invalid Params"); return; }
    BodyInterface &bodyInterface = joltCtx->physicsSystem->GetBodyInterface();
    BodyID id(bodyId);
    RVec3 pos = bodyInterface.GetPosition(id);
    outPos[0] = (float)pos.GetX();
    outPos[1] = (float)pos.GetY();
    outPos[2] = (float)pos.GetZ();
    Quat rot = bodyInterface.GetRotation(id);
    outRot[0] = (float)rot.GetX();
    outRot[1] = (float)rot.GetY();
    outRot[2] = (float)rot.GetZ();
    outRot[3] = (float)rot.GetW();
}


#ifdef __cplusplus
}
#endif

#endif // APP_DRIVER_PHYSICS_BACKEND == DRIVER_PHYSICS_BACKEND_JOLT
