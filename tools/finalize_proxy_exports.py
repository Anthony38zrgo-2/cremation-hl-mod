"""Restore exact legacy decorated names in the newly built DLL's export table.

Only this project's DLL is processed. Code addresses and imported DLLs are not
changed. GoldSrc requires the name array and contiguous strings inside .rdata.
"""
from pathlib import Path
import pefile, struct, json
source=Path(__file__).resolve().parents[1]/'source'
path=source/'build/cremation.dll'
mapping=json.loads((source/'export-names.json').read_text())
pe=pefile.PE(str(path)); raw=bytearray(path.read_bytes())
symbols=sorted((mapping.get(s.name.decode(),s.name.decode()),s.ordinal-pe.DIRECTORY_ENTRY_EXPORT.struct.Base) for s in pe.DIRECTORY_ENTRY_EXPORT.symbols if s.name)
offset=raw.find(b'CREMATION_EXPORT_RESERVE_BEGIN\0')
assert offset>=0,'Missing linked .rdata reserve'
rva=pe.get_rva_from_offset(offset)
rdata=next(s for s in pe.sections if s.Name.rstrip(b'\0')==b'.rdata')
assert rdata.PointerToRawData<=offset<rdata.PointerToRawData+rdata.SizeOfRawData
count=len(symbols)
section=bytearray(count*6)
for i,(name,ordinal) in enumerate(symbols):
    struct.pack_into('<I',section,i*4,rva+len(section))
    struct.pack_into('<H',section,count*4+i*2,ordinal)
    section.extend(name.encode()+b'\0')
assert len(section)<=32768
exports=pe.DIRECTORY_ENTRY_EXPORT.struct
struct.pack_into('<I',raw,exports.get_field_absolute_offset('AddressOfNames'),rva)
struct.pack_into('<I',raw,exports.get_field_absolute_offset('AddressOfNameOrdinals'),rva+count*4)
raw[offset:offset+len(section)]=section
pe.close()
path.write_bytes(raw)
check=pefile.PE(str(path))
assert {s.name.decode() for s in check.DIRECTORY_ENTRY_EXPORT.symbols if s.name}=={n for n,o in symbols}
print('Finalized exact legacy export names inside .rdata:',count)
