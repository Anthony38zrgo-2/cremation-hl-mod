#pragma once

#include <cstddef>
#include <cstdint>

// Verified x86 Ravenholm interface points. Keep donor-specific layout facts in
// one place; gameplay modules should refer to a named slot or RVA.
namespace ravenholm_abi {
namespace slot {
constexpr std::size_t Classify=8;
constexpr std::size_t TraceAttack=10;
constexpr std::size_t TakeDamage=11;
constexpr std::size_t Killed=13;
constexpr std::size_t BloodColor=14;
constexpr std::size_t MyMonsterPointer=17;
constexpr std::size_t HandleAnimEvent=57;
constexpr std::size_t Relationship=63;
constexpr std::size_t MoveExecute=73;
constexpr std::size_t SetActivity=93;
constexpr std::size_t GetDeathActivity=114;
}

namespace private_offset {
// CBaseMonster fields read by the David animation validation probe.
constexpr std::uintptr_t MonsterFrameRate=0x64;
constexpr std::uintptr_t MonsterAnimationEnd=0x70;
}

constexpr std::uintptr_t ResetSequenceInfoRva=0x6bf0;
constexpr std::uintptr_t GetDeathActivityRva=0x1a630;
constexpr std::uintptr_t ZombieTraceAttackRva=0x1c390;
constexpr std::uintptr_t ZombieHandleAnimEventRva=0xa7290;
constexpr std::uintptr_t ZombieMoveExecuteRva=0x55fa0;
constexpr std::uintptr_t ZombieSetActivityRva=0x54a90;
constexpr std::uintptr_t MonsterRelationshipRva=0x565f0;

constexpr std::uintptr_t MultiDamageEntityOffset=0x1282dc;
constexpr std::uintptr_t MultiDamageAmountOffset=0x1282e0;
constexpr std::uintptr_t MultiDamageBitsOffset=0x1282e4;

inline unsigned char* address(void* module,std::uintptr_t rva) {
    return static_cast<unsigned char*>(module)+rva;
}
}
