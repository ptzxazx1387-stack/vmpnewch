#include "context.h"
#include "../memory/offsets.h"
#include "../memory/resolver.h"

namespace GameContext {

static std::vector<EntityBase> g_entities;
static EntityBase g_local(0);

static uint32_t g_ent_count = 0;

static constexpr uint64_t HASH_PED = 0x86DD261F0257748DULL;

static uint64_t GetPoolFromHash(uint64_t hash) {
    typedef uint64_t (__fastcall* GetPoolBaseFn)(uint64_t);
    auto fn = (GetPoolBaseFn)(g::GtaCore + 0x9AFF0);
    return fn ? fn(hash) : 0;
}

void UpdateEntities() {
    g_entities.clear();
    if (!g::GtaCore) return;
    uint64_t ped = GetPoolFromHash(HASH_PED);
    if (!ped) return;

    auto* entries = *(void**)(ped + offsets::POOL_ENTRY);
    auto* flags   = *(void**)(ped + offsets::POOL_FLAG);
    uint32_t cnt  = *(uint32_t*)(ped + offsets::POOL_COUNT);
    uint32_t sz   = *(uint32_t*)(ped + offsets::POOL_ESIZE);

    for (uint32_t i = 0; i < cnt; i++) {
        if ((*(uint8_t*)((uintptr_t)flags + i) & 0x80) == 0) continue;
        uintptr_t ea = (uintptr_t)entries + i * sz;
        EntityBase ent(ea);
        if (ent.GetType() != 1) continue;
        g_entities.push_back(ent);
    }
    g_ent_count = (int)g_entities.size();
}

EntityBase* GetLocalPlayer() {
    static EntityBase loc(0);
    uint64_t mgr = *(uint64_t*)(g::PlayerMgr);
    if (!mgr) return &loc;
    uint64_t pi = *(uint64_t*)(mgr + 0x180);
    if (!pi) return &loc;
    uint64_t ped = *(uint64_t*)(pi + 0x00);
    loc = EntityBase(ped);
    return &loc;
}

std::vector<EntityBase>& GetAllEntities() { return g_entities; }

} // namespace GameContext