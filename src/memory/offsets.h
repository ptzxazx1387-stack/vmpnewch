#pragma once
#include <cstdint>

//=== VMP Cheat Offsets — gta-core-five.dll @ 0x180000000 ====================
// Verified from IDA Pro analysis of VMP client (2026-07-29)
// Unverified offsets marked with TODO — need runtime confirmation.

namespace offsets {

//=== Pool system — gta-core-five.dll ========================================
constexpr uintptr_t GtaCore_GetPoolBase   = 0x18009AFF0;
constexpr uintptr_t GtaCore_GetPools      = 0x18009B0A0;
constexpr uintptr_t GtaCore_PoolHashTable = 0x180146488;
constexpr uintptr_t GtaCore_PoolHashMask  = 0x1801464A0;
constexpr uintptr_t GtaCore_PoolRegistry  = 0x1801464F0;

//=== Frame hooks (fwEvent globals) ==========================================
constexpr uintptr_t GtaCore_OnInitStart  = 0x1801451E0;
constexpr uintptr_t GtaCore_OnInitEnd    = 0x1801451E8;
constexpr uintptr_t GtaCore_OnGameFrame  = 0x1801451F0;
constexpr uintptr_t GtaCore_OnMainFrame  = 0x1801451F8;

//=== String references (for pattern finding) =================================
constexpr uintptr_t STR_netPlayerMgrBase = 0x1800E8DE8;  // "netPlayerMgrBase"
constexpr uintptr_t STR_CNetworkPlyrMgr  = 0x1800D5820;  // "CNetworkPlayerMgr"

//=== Entity (CEntity / CPhysical base) ===========================================
constexpr uint32_t ENTITY_MATRIX  = 0x60;
constexpr uint32_t ENTITY_POS_X   = 0x90;
constexpr uint32_t ENTITY_POS_Y   = 0x94;
constexpr uint32_t ENTITY_POS_Z   = 0x98;
constexpr uint32_t ENTITY_TYPE    = 0xCB;   // & 0x1F

// [TODO-verify] Navigation/rotation data (public game angles — what other players see)
// These are at or near the shared entity coordinates buffer
constexpr uint32_t ENTITY_NAVDATA = 0xE0;

//=== CPed specific =========================================================
constexpr uint32_t ENTITY_HEALTH  = 0x2C8;
constexpr uint32_t ENTITY_ARMOR   = 0x2CC;
constexpr uint32_t ENTITY_MAXHLTH = 0x2D0;

// [TODO-verify] Ped navigation angles (actual view direction, may differ from entity rotation)
constexpr uint32_t PED_VIEW_ANGLE_X = 0x60;   // Pitch/heading
constexpr uint32_t PED_VIEW_ANGLE_Y = 0x64;

// [TODO-verify] Bone data chain (CPed → skeleton → bones)
// GTA V standard: Chairs +0x430 → crFragmentProcesor +0x20 → boneEntry[0] +0x30
constexpr uint32_t PED_BONE_COMP   = 0x430;   // → crSkeleton component pointer
constexpr uint32_t BONE_COMP_COUNT = 0x18;    // bone count inside component
constexpr uint32_t BONE_COMP_ARRAY = 0x20;    // → bone data array
constexpr uint32_t BONE_ENTRY_SIZE = 64;     // 4x4 mat per bone entry

// [TODO-verify] Weapon manager (CPed weapon / ammo info)
constexpr uint32_t PED_WEAPON_MGR  = 0x10A8;  // → CWeaponInfoManager
constexpr uint32_t WEAPON_MGR_CUR  = 0x50;    // offset to current weapon hash (0 = unarmed)

// Vehicle (for IsInVehicle / GetVehicle)
constexpr uint32_t PED_VEHICLE     = 0xD28;

//=== pool (atPoolBase) structure ============================================
constexpr uint32_t POOL_ENTRY   = 0x00;
constexpr uint32_t POOL_FLAGS   = 0x08;
constexpr uint32_t POOL_COUNT   = 0x10;
constexpr uint32_t POOL_ESIZE   = 0x14;

//=== Vehicle specific ======================================================
constexpr uint32_t VEH_VELOCITY     = 0x320;
constexpr uint32_t VEH_PASSENGERS   = 0x4B8;
constexpr uint32_t VEH_DRIVER       = 0x4B0;

//=== Player / PlayerInfo / NetworkPlayerMgr =================================
// networkPlayerMgrBase → CNetworkPlayerMgr
// [TODO-verify] Offsets below are GTA V FiveM standard; may shift version to version
constexpr uint32_t NETPMGR_LOCAL       = 0x180;   // m_localPlayer (returned PlayerInfo*)
constexpr uint32_t NETPMGR_PLAYEREN_ARY = 0x188; // m_players (for enum all players)
constexpr uint32_t NETPMGR_MAX_PLAYERS = 0x1B0;  // m_maxPlayers
//
// [TODO-verify] CPlayerInfo
constexpr uint32_t PLAYERINFO_PED   = 0x00;    // CPed* — null if not spawned
constexpr uint32_t PLAYERINFO_ID    = 0x08;    // int playerId
constexpr uint32_t PLAYERINFO_NAME  = 0x0C;    // const char* name (may also be at +0x7C)
constexpr uint32_t PLAYERINFO_IPL   = 0x18;    // int internal player index

//=== Camera / Addressing ====================================================
// [TODO-verify] CViewportGame → camera view+projection matrix
// Found by pattern: "48 8B 05 ?? ?? ?? ?? 48 8B D9 48 85 ED ?? 8B 0D" near viewport referencing code
constexpr uint32_t VIEWPORT_MATRIX = 0x258; // float[16] view-projection matrix

//=== Globals (runtime-resolved, set by resolver.cpp) ==========================
// Defined in resolver.cpp via pattern scan
extern uintptr_t g_ViewMatrix;    // resolved global for float[16] matrix
extern uintptr_t g_PlayerMgr;     // → CNetworkPlayerMgr
extern uintptr_t g_Camera;        // → game camera

} // namespace offsets