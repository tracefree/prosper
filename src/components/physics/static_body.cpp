#include <components/physics/static_body.h>

#include <physics.h>
#include <core/node.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

void StaticBody::initialize() {
    JPH::BodyCreationSettings body_settings(
        shape,
        JPH::Vec3(
            node->get_global_transform().position.x,
            node->get_global_transform().position.y,
            node->get_global_transform().position.z
        ),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Static,
        Layers::NON_MOVING
    );
    body_id = Physics::body_interface->CreateAndAddBody(
        body_settings, 
        JPH::EActivation::Activate
    );
    Physics::body_interface->SetFriction(body_id, 0.2f);
}

void StaticBody::cleanup() {
    Physics::body_interface->RemoveBody(body_id);
	Physics::body_interface->DestroyBody(body_id);
}