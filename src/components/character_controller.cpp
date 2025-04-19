#include <components/character_controller.h>

#include <input.h>
#include <physics.h>
#include <components/camera.h>
#include <components/animation_player.h>
#include <core/node.h>
#include <core/scene_graph.h>
#include <yaml.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

#include <print>

extern SceneGraph scene;


void CharacterController::update(double delta) {
    JPH::RVec3 current_position = character->GetPosition();
    character->SetLinearVelocity(JPH::Vec3(target_velocity.x, target_velocity.y, target_velocity.z));
    JPH::CharacterVirtual::ExtendedUpdateSettings update_settings;
    character->ExtendedUpdate(
        float(delta),
        JPH::Vec3(0.0, -9.81f, 0.0),
        update_settings,
        Physics::physics_system.GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
        Physics::physics_system.GetDefaultLayerFilter(Layers::MOVING),
        { },
        { },
        *Physics::temp_allocator
    );
    JPH::RVec3 new_position = character->GetPosition();
    node->set_position(Vec3(new_position.GetX(), new_position.GetY(), new_position.GetZ()));

    JPH::RVec3 actual_velocity = new_position - current_position;
    if (actual_velocity.GetY() <= 0.01 && target_velocity.y > 0.0) {
        target_velocity.y = 0.0;
    }
}

void CharacterController::initialize() {
    auto shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0.0, 0.9, 0.0), JPH::Quat::sIdentity(), new JPH::CapsuleShape(0.6f, 0.3f)).Create().Get();
    JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
    settings->mMaxSlopeAngle = 0.0;
    settings->mShape = shape;
    settings->mInnerBodyShape = shape;
    settings->mInnerBodyLayer = Layers::MOVING;
    settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -0.3f);
    character = new JPH::CharacterVirtual(settings, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), 0, &Physics::physics_system);
    auto camera = scene.camera->get_component<Camera>();
    camera->body_to_exclude = character->GetInnerBodyID();
    camera->follow_target = node;
    camera->yaw = M_PI_2;
}

COMPONENT_FACTORY_IMPL(CharacterController, character_controller) {}