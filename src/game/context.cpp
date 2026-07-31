#include "context.h"
#include "../memory/offsets.h"
#include "../memory/resolver.h"
#include "../memory/scanner.h"

namespace GameContext {

static std::vector<EntityBase> g_entities;
static EntityBase g_local(0);

void UpdateEntities() {
    g_entities.clear();
    if (!g::GtaCore) return;

    // Get ped pool via GetPoolBase(FNV64("CPed"))
    uint64_t ped = g::GetPoolPtrFromHash(g::Fnv64("CPed"));
    if (!ped) return;

    auto* entries = *(void**)(ped + offsets::POOL_ENTRY);
    auto* flags   = *(uint8_t**)(ped + offsets::POOL_FLAGS);
    uint32_t cnt  = *(uint32_t*)(ped + offsets::POOL_COUNT);
    uint32_t sz   = *(uint32_t*)(ped + offsets::POOL_ESIZE);

    if (!entries || !flags || !cnt || !sz) return;

    for (uint32_t i = 0; i < cnt && i < 256; i++) {
        // Valid slot: High bit set AND low bits = type info
        if (!(flags[i] & 0x80)) continue;    // slot not in use
        uintptr_t ea = (uintptr_t)entries + (uintptr_t)i * sz;
        if (ea < 0x1000) continue;           // null/invalid address
        EntityBase ent(ea);
        if (ent.GetType() != 1) continue;    // ped only
        g_entities.push_back(ent);
    }
}

EntityBase* GetLocalPlayer() {
    static EntityBase loc(0);

    // Resolve local player via CNetworkPlayerMgr
    if (!g::PlayerMgr || g::PlayerMgr < 0x1000) return &loc;

    uint64_t mgr = *(uint64_t*)g::PlayerMgr;          // CNetworkPlayerMgr*
    if (!mgr || mgr < 0x1000) return &loc;

    uint64_t pi = *(uint64_t*)(mgr + offsets::NETPMGR_LOCAL); // CPlayerInfo*
    if (!pi || pi < 0x1000) return &loc;

    uint64_t ped = *(uint64_t*)(pi + offsets::PLAYERINFO_PED); // CPed*
    if (!ped || ped < 0x1000) return &loc;

    loc = EntityBase(ped);
    return &loc;
}

std::vector<EntityBase>& GetAllEntities() {
    return g_entities;
}

} // namespace GameContext