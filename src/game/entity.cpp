#include "entity.h"
#include "../memory/offsets.h"
#include "../memory/resolver.h"

// Direct memory access (we're in-process via injection)
template<typename T>
static inline T read_mem(uintptr_t addr) { return *(T*)addr; }

//=== EntityBase =============================================================

Vec3 EntityBase::GetPos() const {
    return {
        read_mem<float>(addr + offsets::ENTITY_POS_X),
        read_mem<float>(addr + offsets::ENTITY_POS_Y),
        read_mem<float>(addr + offsets::ENTITY_POS_Z)
    };
}

float EntityBase::GetHealth() const {
    return read_mem<float>(addr + offsets::ENTITY_HEALTH);
}

uint8_t EntityBase::GetType() const {
    return read_mem<uint8_t>(addr + offsets::ENTITY_TYPE) & 0x1F;
}

bool EntityBase::IsValid() const {
    return addr > 0x1000 && GetHealth() > 0.0f;
}

Matrix4x4 EntityBase::GetMatrix() const {
    return *(Matrix4x4*)(addr + offsets::ENTITY_MATRIX);
}

//=== CPed ===================================================================

float CPed::GetArmor() const {
    return read_mem<float>(addr + offsets::ENTITY_ARMOR);
}

uint64_t CPed::GetWeaponHash() const {
    // Weapon manager chain: CPed + PED_WEAPON_MGR → CWeaponManager → current hash
    uint64_t wpnMgr = read_mem<uint64_t>(addr + offsets::PED_WEAPON_MGR);
    if (!wpnMgr || wpnMgr < 0x1000) return 0;
    return read_mem<uint64_t>(wpnMgr + offsets::WEAPON_MGR_CUR);
}

Vec3 CPed::GetBonePos(int boneId) const {
    // Bone chain: CPed + BoneOffset → crSkeleton + 0x20 → BoneEntry[boneId]
    if (!g::BoneOffset) return GetPos();

    uint64_t skel = read_mem<uint64_t>(addr + g::BoneOffset);
    if (!skel || skel < 0x1000) return GetPos();

    uint64_t boneArr = read_mem<uint64_t>(skel + offsets::BONE_COMP_ARRAY);
    if (!boneArr || boneArr < 0x1000) return GetPos();

    // Each bone is two 4×4 matrices concatenated (64 bytes each)
    // First matrix: bone-space transform
    // Second matrix: world-space bone position starts at boneArr + boneId*BONE_ENTRY_SIZE + 16
    uintptr_t entry = boneArr + (uintptr_t)boneId * offsets::BONE_ENTRY_SIZE;

    // Translation vector is at +16 (4 floats = x,y,z,w) in the second matrix
    // First 16 bytes = quaternion/rotation; bytes 16-31 = translation
    return *(Vec3*)(entry + 16);
}

bool CPed::IsInVehicle() const {
    uint64_t veh = read_mem<uint64_t>(addr + offsets::PED_VEHICLE);
    return veh > 0x1000;
}

uint64_t CPed::GetVehicle() const {
    return read_mem<uint64_t>(addr + offsets::PED_VEHICLE);
}

//=== Vehicle ================================================================

float Vehicle::GetSpeed() const {
    Vec3 vel = *(Vec3*)(addr + offsets::VEH_VELOCITY);
    return vel.length();
}

int Vehicle::GetNumPassengers() const {
    return read_mem<int>(addr + offsets::VEH_PASSENGERS);
}

uint64_t Vehicle::GetDriver() const {
    return read_mem<uint64_t>(addr + offsets::VEH_DRIVER);
}