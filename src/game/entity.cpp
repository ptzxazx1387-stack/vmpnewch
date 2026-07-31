#include "entity.h"
#include "../memory/offsets.h"
#include "../memory/resolver.h"
#include <cstring>

// Simple RPM since we're in-process — direct pointer dereference
template<typename T>
T read_mem(uintptr_t addr) { return *(T*)addr; }

// EntityBase methods
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
    return addr != 0 && GetHealth() > 0.0f;
}

Matrix4x4 EntityBase::GetMatrix() const {
    return *(Matrix4x4*)(addr + offsets::ENTITY_MATRIX);
}

// CPed
float CPed::GetArmor() const {
    return read_mem<float>(addr + offsets::ENTITY_ARMOR);
}

uint64_t CPed::GetWeaponHash() const {
    // Weapon manager chain: CPed + 0x10A8 → CWeaponManager + 0x50
    uint64_t weaponMgr = read_mem<uint64_t>(addr + 0x10A8);
    if (!weaponMgr) return 0;
    return read_mem<uint32_t>(weaponMgr + 0x50);
}

Vec3 CPed::GetBonePos(int boneId) const {
    // Bone data chain: CPed + boneComponent → boneArray[boneId]
    if (!g::BoneOffset) return GetPos();
    uint64_t boneComp = read_mem<uint64_t>(addr + g::BoneOffset);
    if (!boneComp) return GetPos();
    uint64_t boneData = read_mem<uint64_t>(boneComp + 0x20);
    if (!boneData) return GetPos();
    // Each bone entry = 4x4 matrix (64 bytes), we take translation column
    return *(Vec3*)(boneData + boneId * 64 + 16); // column 3 = translation
}

bool CPed::IsInVehicle() const {
    // Check: CPed+some offset has vehicle pointer
    uint64_t veh = read_mem<uint64_t>(addr + 0xD28);
    return veh != 0;
}

uint64_t CPed::GetVehicle() const {
    return read_mem<uint64_t>(addr + 0xD28);
}

// Vehicle
float Vehicle::GetSpeed() const {
    Vec3 vel = *(Vec3*)(addr + 0x320);
    return vel.length(); // m/s, ~3.6 * = km/h
}

int Vehicle::GetNumPassengers() const {
    return read_mem<int>(addr + 0x4B8);
}

uint64_t Vehicle::GetDriver() const {
    return read_mem<uint64_t>(addr + 0x4B0);
}