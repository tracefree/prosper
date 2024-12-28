#include <components/physics/static_body.h>

#include <physics.h>
#include <core/node.h>
#include <Jolt/Jolt.h>

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
    /*
    Physics::body_interface->SetRotation(body_id, JPH::Quat(
        node->get_global_transform().rotation.x,
        node->get_global_transform().rotation.y,
        node->get_global_transform().rotation.z,
        node->get_global_transform().rotation.w
    ), JPH::EActivation::Activate); */
}

void StaticBody::update(double delta) {
    // TODO: Anything?
}

void StaticBody::cleanup() {
    Physics::body_interface->RemoveBody(body_id);
	Physics::body_interface->DestroyBody(body_id);
}

std::string StaticBody::get_name() {
    return "StaticBody";
}