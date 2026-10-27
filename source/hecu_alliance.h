#pragma once
#include "ravenholm_abi.h"
// Ravenholm uses the verified GoldSrc Classify ABI at virtual slot 8.
// Share the zombies' faction so native AI relationships work in both directions.
static int __fastcall cremationHecuClassify(void*, void*) { return 7; }
static bool cremationAllyHecu(void* object) {
    if(!object)return false;
    auto table=*reinterpret_cast<void***>(object);
    void* replacement=reinterpret_cast<void*>(&cremationHecuClassify);
    if(table[ravenholm_abi::slot::Classify]==replacement)return true;
    DWORD previous=0;
    if(!VirtualProtect(&table[ravenholm_abi::slot::Classify],sizeof(void*),PAGE_READWRITE,&previous))return false;
    table[ravenholm_abi::slot::Classify]=replacement;
    DWORD ignored=0;VirtualProtect(&table[ravenholm_abi::slot::Classify],sizeof(void*),previous,&ignored);
    return true;
}
