#pragma once
#include <cstdint>

// VMP Cheat Offsets — gta-core-five.dll @ 0x180000000
namespace offsets {

constexpr uintptr_t GtaCore_GetPoolBase   = 0x18009AFF0;
constexpr uintptr_t GtaCore_GetPools      = 0x18009B0A0;
constexpr uintptr_t GtaCore_PoolHashTable = 0x180146488;
constexpr uintptr_t GtaCore_PoolHashMask  = 0x1801464A0;
constexpr uintptr_t GtaCore_PoolRegistry  = 0x1801464F0;
constexpr uintptr_t GtaCore_OnGameFrame   = 0x1801451F0;
constexpr uintptr_t GtaCore_OnMainFrame   = 0x1801451F8;

constexpr uint32_t ENTITY_POS_X   = 0x90;
constexpr uint32_t ENTITY_POS_Y   = 0x94;
constexpr uint32_t ENTITY_POS_Z   = 0x98;
constexpr uint32_t ENTITY_TYPE    = 0xCB;
constexpr uint32_t ENTITY_HEALTH  = 0x2C8;
constexpr uint32_t ENTITY_ARMOR   = 0x2CC;
constexpr uint32_t ENTITY_MATRIX  = 0x60;
constexpr uint32_t ENTITY_NAVDATA = 0xE0;

constexpr uint32_t POOL_ENTRY = 0x00;
constexpr uint32_t POOL_FLAG  = 0x08;
constexpr uint32_t POOL_COUNT = 0x10;
constexpr uint32_t POOL_ESIZE = 0x14;

} // namespace offsets