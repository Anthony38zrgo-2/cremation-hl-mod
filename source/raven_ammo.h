// Discover the donor's player ammo array from its own save-field metadata.
// Do not assume CBasePlayer's private layout matches the Valve SDK.
#pragma once
#include <cstring>
#include <cstdint>
inline int ravenAmmoOffset(HMODULE module) {
    if(!module)return -1;
    auto base=reinterpret_cast<unsigned char*>(module);
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+reinterpret_cast<IMAGE_DOS_HEADER*>(base)->e_lfanew);
    const size_t size=nt->OptionalHeader.SizeOfImage;
    auto named=[&](uintptr_t address,const char* name) {
        uintptr_t low=reinterpret_cast<uintptr_t>(base);
        size_t n=std::strlen(name)+1;
        return address>=low && address+n<=low+size && !std::memcmp(reinterpret_cast<void*>(address),name,n);
    };
    auto section=IMAGE_FIRST_SECTION(nt);
    for(int j=0;j<nt->FileHeader.NumberOfSections;++j) {
        if(!(section[j].Characteristics&IMAGE_SCN_MEM_WRITE))continue;
        size_t end=section[j].VirtualAddress+section[j].Misc.VirtualSize;
        for(size_t i=section[j].VirtualAddress;i+32<=end && i+32<=size;i+=4) {
            auto f=reinterpret_cast<const uint32_t*>(base+i);
            // TYPEDESCRIPTION: FIELD_INTEGER=10, fieldName, fieldOffset,
            // fieldSize=32, flags=0; next player field is m_idrowndmg.
            if(f[0]==10 && f[3]==32 && named(f[1],"m_rgAmmo") && named(f[5],"m_idrowndmg") && f[2]<8192)
                return int(f[2]);
        }
    }
    return -1; // Fail closed on unsupported donor builds.
}
