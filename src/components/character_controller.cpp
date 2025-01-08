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

class CharacterBodyFilter final : public JPH::BodyFilter {
    public:
    JPH::BodyID* body_to_exclude;
    virtual bool ShouldCollide(const JPH::BodyID& in_body_id) const override {
        //std::println("Coll? {}", in_body_id.GetIndex());
        if (body_to_exclude == nullptr) {
            return true;
        }
        return (in_body_id != *body_to_exclude);
    }
};

CharacterBodyFilter character_body_filter;

void CharacterController::process_input(SDL_Event& event) {

}

void CharacterController::update(double delta) {
    if (delta == 0.0) { return;}
    auto animation_player = node->get_component<AnimationPlayer>();

    const bool grounded = (character->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround);
    
    if (!grounded) {
        target_velocity.y -= 9.81f * float(delta);
        if (animation_player->current_animation != "Air-loop") {
            animation_player->play("Air-loop");
        }
    } else {
       target_velocity.y = (Input::is_pressed(SDL_SCANCODE_SPACE)) ? 4.0f : 0.0f;
    }
    
    const float speed = Input::is_pressed(SDL_SCANCODE_LSHIFT) ? 5.0f : 3.0f;
    const Vec2 movement_input = enabled ? Input::get_movement_input() : Vec2(0.0f);
    const Vec3 movement_direction = Mat3(scene.camera->get_component<Camera>()->get_horizontal_rotation_matrix()) * Vec3(movement_input.x, 0.0f, -movement_input.y);
    const Vec3 planar_velocity = movement_direction * speed;
    if (glm::length(planar_velocity) > 0.0f) {
        Vec2 current_planar_velocity(target_velocity.x, target_velocity.z);
        Vec2 target_planar_velocity(planar_velocity.x, planar_velocity.z);
        Vec2 new_planar_velocity = Math::interpolate(current_planar_velocity, target_planar_velocity, 0.05f, delta);
        target_velocity.x = new_planar_velocity.x;
        target_velocity.z = new_planar_velocity.y;

        glm::quat target_rotation = glm::quatLookAt(-movement_direction, Vec3(0.0f, 1.0f, 0.0f));
        float weight = (1.0f - std::exp(-(float(delta)/0.075f)));
        node->set_rotation(glm::slerp(node->get_rotation(), target_rotation, weight));
    } else if (grounded) {
        target_velocity.x = 0.0f;
        target_velocity.z = 0.0f;
    }
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
    if (!grounded) return;

    if (std::abs(movement_input.x) > 0 || std::abs(movement_input.y) > 0) {
        if (animation_player->current_animation != "Run-loop" && !Input::is_pressed(SDL_SCANCODE_LSHIFT)) {
            animation_player->play("Run-loop");
        } else if (animation_player->current_animation != "Sprint-loop" && Input::is_pressed(SDL_SCANCODE_LSHIFT)) {
            animation_player->play("Sprint-loop");
        }
    } else if (animation_player->current_animation != "Idle-loop") {
        animation_player->play("Idle-loop");
    }
}

void CharacterController::initialize() {
    auto shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0.0, 0.8, 0.0), JPH::Quat::sIdentity(), new JPH::CapsuleShape(0.5f, 0.3f)).Create().Get();
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

void CharacterController::cleanup() {
    // TODO: Needed?
}

COMPONENT_FACTORY_IMPL(CharacterController, character_controller) {

}