#include <physics.h>
#include <math.h>
#include <rendering/types.h>

#include <iostream>
#include <cstdarg>
#include <thread>
#include <print>

JPH_SUPPRESS_WARNINGS

using namespace JPH::literals;

extern PerformanceStats gStats;

Physics::BPLayerInterfaceImpl				Physics::broad_phase_layer_interface;
Physics::ObjectVsBroadPhaseLayerFilterImpl	Physics::object_vs_broadphase_layer_filter;
Physics::ObjectLayerPairFilterImpl			Physics::object_vs_object_layer_filter;
JPH::BodyInterface* Physics::body_interface;
JPH::PhysicsSystem Physics::physics_system;
JPH::TempAllocatorImpl* Physics::temp_allocator;
JPH::JobSystemThreadPool* Physics::job_system;

constexpr float Physics::cDeltaTime 	= 1.0f / 60.0f;
constexpr uint cMaxBodies 				= 1024;
constexpr uint cNumBodyMutexes 			= 0;
constexpr uint cMaxBodyPairs 			= 1024;
constexpr uint cMaxContactConstraints 	= 1024;

// --- BPLayerInterfaceImpl ---
Physics::BPLayerInterfaceImpl::BPLayerInterfaceImpl() {
	// Create a mapping table from object to broad phase layer
	mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
	mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
}

uint Physics::BPLayerInterfaceImpl::GetNumBroadPhaseLayers() const {
	return BroadPhaseLayers::NUM_LAYERS;
}

JPH::BroadPhaseLayer Physics::BPLayerInterfaceImpl::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const {
	JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
	return mObjectToBroadPhase[inLayer];
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char* Physics::BPLayerInterfaceImpl::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const {
	switch ((JPH::BroadPhaseLayer::Type)inLayer) {
		case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
		case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
		default:									 JPH_ASSERT(false); return "INVALID";
	}
}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED


// --- ObjectVsBroadPhaseLayerFilterImpl ---
bool Physics::ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const {
	switch (inLayer1) {
		case Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::MOVING:
			return true;
		default:
			JPH_ASSERT(false);
			return false;
	}
}

// --- ObjectLayerPairFilterImpl ---
bool Physics::ObjectLayerPairFilterImpl::ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const {
	switch (inObject1) {
		case Layers::NON_MOVING:
			return inObject2 == Layers::MOVING; // Non moving only collides with moving
		case Layers::MOVING:
			return true; // Moving collides with everything
		default:
			JPH_ASSERT(false);
			return false;
	}
}

class MyContactListener : public JPH::ContactListener {
public:
	virtual JPH::ValidateResult	OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override
	{
		//std::cout << "Contact validate callback" << std::endl;
		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
	{
		//std::cout << "A contact was added" << std::endl;
	}

	virtual void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
	{
		//std::cout << "A contact was persisted" << std::endl;
	}

	virtual void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override
	{
		//std::cout << "A contact was removed" << std::endl;
	}
};


class MyBodyActivationListener : public JPH::BodyActivationListener
{
public:
	virtual void OnBodyActivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override
	{
		//std::cout << "A body got activated" << std::endl;
	}

	virtual void OnBodyDeactivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override
	{
		//std::cout << "A body went to sleep" << std::endl;
	}
};

MyBodyActivationListener body_activation_listener;
MyContactListener contact_listener;


void Physics::initialize() {
	JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
	JPH::RegisterTypes();

	Physics::temp_allocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
	Physics::job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

	physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, Physics::broad_phase_layer_interface, Physics::object_vs_broadphase_layer_filter, Physics::object_vs_object_layer_filter);
	physics_system.SetBodyActivationListener(&body_activation_listener);
	physics_system.SetContactListener(&contact_listener);

	body_interface = &physics_system.GetBodyInterface();
	
	physics_system.OptimizeBroadPhase();
}

void Physics::update() {
	// TODO: Fixed timestep
	physics_system.Update((gStats.frametime) / 1000.0f, 1, temp_allocator, job_system);
}

void Physics::cleanup() {
	delete Physics::job_system;
	Physics::job_system = nullptr;

	delete Physics::temp_allocator;
	Physics::temp_allocator = nullptr;

	JPH::UnregisterTypes();
	delete JPH::Factory::sInstance;
	JPH::Factory::sInstance = nullptr;
}