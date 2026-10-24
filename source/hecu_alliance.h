#pragma once
// Ravenholm uses the verified GoldSrc Classify ABI at virtual slot 8.
// Share the zombies' faction so native AI relationships work in both directions.
static int __fastcall cremationHecuClassify(void*, void*) { return 7; }
static bool cremationAllyHecu(void* object) {
    if(!object)return false;
    auto table=*reinterpret_cast<void***>(object);
    void* replacement=reinterpret_cast<void*>(&cremationHecuClassify);
    if(table[8]==replacement)return true;
    DWORD previous=0;
    if(!VirtualProtect(&table[8],sizeof(void*),PAGE_READWRITE,&previous))return false;
    table[8]=replacement;
    DWORD ignored=0;VirtualProtect(&table[8],sizeof(void*),previous,&ignored);
    return true;
}
