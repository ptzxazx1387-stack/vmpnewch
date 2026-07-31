#pragma once
#include <cstdint>
#include "../utils/vector.h"

// Thin wrappers over remote memory — no actual C++ objects
struct EntityBase {
    uint64_t addr;
    explicit EntityBase(uint64_t a) : addr(a) {}

    Vec3 GetPos() const;
    float GetHealth() const;
    uint8_t GetType() const;
    bool IsValid() const;

    // World transform matrix (entity coordinate system)
    Matrix4x4 GetMatrix() const;
};

struct CPed : EntityBase {
    using EntityBase::EntityBase;
    float GetArmor() const;
    uint64_t GetWeaponHash() const;
    Vec3 GetBonePos(int boneId) const;
    bool IsInVehicle() const;
    uint64_t GetVehicle() const;
};

struct Vehicle : EntityBase {
    using EntityBase::EntityBase;
    float GetSpeed() const;
    int GetNumPassengers() const;
    uint64_t GetDriver() const;
};