#include <components/physics/rigid_body.h>

#include <physics.h>
#include <core/node.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

void RigidBody::initialize() {
    JPH::BodyCreationSettings body_settings(
        shape,
        JPH::Vec3(
            node->get_global_transform().position.x,
            node->get_global_transform().position.y,
            node->get_global_transform().position.z
        ),
        JPH::Quat::sIdentity(), // TODO: Set rotation
        JPH::EMotionType::Dynamic,
        Layers::MOVING
    );
    JPH::MassProperties mass_properties;
    mass_properties.ScaleToMass(1.0f);
    body_settings.mMassPropertiesOverride = mass_properties;
    body_id = Physics::body_interface->CreateAndAddBody(
        body_settings, 
        JPH::EActivation::Activate
    );
    set_linear_velocity(linear_velocity);   
    Physics::body_interface->SetFriction(body_id, 0.2f);
    Physics::body_interface->SetRestitution(body_id, 0.05f);
    Physics::body_interface->SetRotation(body_id, JPH::Quat(
        node->get_global_transform().rotation.x,
        node->get_global_transform().rotation.y,
        node->get_global_transform().rotation.z,
        node->get_global_transform().rotation.w
    ), JPH::EActivation::Activate);
}

void RigidBody::set_linear_velocity(Vec3 p_linear_velocity) {
    linear_velocity = p_linear_velocity;
    Physics::body_interface->SetLinearVelocity(body_id, JPH::Vec3(
        linear_velocity.x,
        linear_velocity.y,
        linear_velocity.z
    ));
}

Vec3 RigidBody::get_linear_velocity() const {
    // TODO: Sync with physics server
    return linear_velocity;
}

void RigidBody::update(double delta) {
    auto position = Physics::body_interface->GetCenterOfMassPosition(body_id);
    node->set_position(position.GetX(), position.GetY(), position.GetZ());

    auto rotation = Physics::body_interface->GetRotation(body_id);
    node->set_rotation(Quaternion(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ()));
}

void RigidBody::cleanup() {
    Physics::body_interface->RemoveBody(body_id);
	Physics::body_interface->DestroyBody(body_id);
}