#pragma once
// Strict, deliberately small JSON reader for flat numeric/boolean weapon files.
// No external runtime dependency. Parse and validate the whole file before writes.
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iterator>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cctype>
#include "ricochet_balance.h"
#include "bounce_shotgun_balance.h"
namespace weapon_config {
struct Value { double number; bool boolean; };
struct Reader {
    const std::string& text; size_t pos=0;
    void space(){while(pos<text.size() && (text[pos]==' '||text[pos]=='\n'||text[pos]=='\r'||text[pos]=='\t'))++pos;}
    bool eat(char c){space();if(pos<text.size()&&text[pos]==c){++pos;return true;}return false;}
    bool digits(){size_t start=pos;while(pos<text.size()&&text[pos]>='0'&&text[pos]<='9')++pos;return pos>start;}
    bool value(Value& out) {
        space();
        for(auto word:{"true","false"})if(text.compare(pos,std::strlen(word),word)==0){pos+=std::strlen(word);out={word[0]=='t'?1.:0.,true};return true;}
        size_t start=pos;
        if(pos<text.size()&&text[pos]=='-')++pos;
        if(pos>=text.size())return false;
        if(text[pos]=='0')++pos;else if(text[pos]>='1'&&text[pos]<='9'){if(!digits())return false;}else return false;
        if(pos<text.size()&&text[pos]=='.'){++pos;if(!digits())return false;}
        if(pos<text.size()&&(text[pos]=='e'||text[pos]=='E')){++pos;if(pos<text.size()&&(text[pos]=='+'||text[pos]=='-'))++pos;if(!digits())return false;}
        out={std::strtod(text.substr(start,pos-start).c_str(),nullptr),false};return std::isfinite(out.number);
    }
    bool object(std::map<std::string,Value>& out) {
        // UTF-8 BOM from Windows editors is accepted.
        if(text.compare(0,3,"\xef\xbb\xbf")==0)pos=3;
        if(!eat('{'))return false;
        if(!eat('}'))for(;;) {
            if(!eat('"'))return false;
            size_t start=pos;
            while(pos<text.size() && ((text[pos]>='a'&&text[pos]<='z') || text[pos]=='_' || (text[pos]>='0'&&text[pos]<='9')))++pos;
            std::string key=text.substr(start,pos-start);
            if(key.empty()||pos>=text.size()||text[pos]!='"')return false;
            ++pos;if(!eat(':'))return false;
            Value v{};if(!value(v)||!out.emplace(key,v).second)return false;
            if(eat('}'))break;
            if(!eat(','))return false;
        }
        space();return pos==text.size();
    }
};
struct Field {
    const char* key; void* address; double low,high; enum Type {Integer,Float,Boolean} type;
    Field(const char* k,int* p,double l,double h):key(k),address(p),low(l),high(h),type(Integer){}
    Field(const char* k,float* p,double l,double h):key(k),address(p),low(l),high(h),type(Float){}
    Field(const char* k,bool* p,double l,double h):key(k),address(p),low(l),high(h),type(Boolean){}
    double read()const{return type==Integer?double(*static_cast<int*>(address)):type==Float?double(*static_cast<float*>(address)):double(*static_cast<bool*>(address));}
    void write(double n)const{if(type==Integer)*static_cast<int*>(address)=int(n);else if(type==Float)*static_cast<float*>(address)=float(n);else *static_cast<bool*>(address)=n!=0;}
};
#include "weapon_config_bindings.inc"
inline bool apply(const std::string& json,const std::vector<Field>& fields,std::string& error) {
    std::map<std::string,Value> values;
    if(!Reader{json}.object(values)){error="invalid JSON (flat object; numeric/boolean values; no duplicate keys)";return false;}
    for(const auto& entry:values) {
        const Field* field=nullptr;for(const auto& f:fields)if(entry.first==f.key){field=&f;break;}
        if(!field){error="unknown key: "+entry.first;return false;}
        double n=entry.second.number;
        if((field->type==Field::Boolean)!=entry.second.boolean || n<field->low || n>field->high || (field->type==Field::Integer&&n!=std::floor(n))) {
            error="invalid type/range: "+entry.first;return false;
        }
    }
    for(const auto& entry:values)for(const auto& f:fields)if(entry.first==f.key)f.write(entry.second.number);
    return true;
}
inline std::string load(const std::string& path,const std::vector<Field>& fields) {
    std::ifstream file(path,std::ios::binary);
    if(!file)return path+": missing/unreadable; retained defaults or last valid settings";
    file.seekg(0,std::ios::end);auto size=file.tellg();file.seekg(0);
    if(size<0||size>16384)return path+": file exceeds 16 KiB; retained last valid settings";
    std::string content((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>()),error;
    if(!apply(content,fields,error))return path+": "+error+"; retained last valid settings";
    return path+": loaded";
}
inline std::vector<std::string> loadAll(const std::string& directory) {
    std::vector<std::string> report;
    report.push_back(load(directory+"/ricochet.json",ricochetFields()));
    report.push_back(load(directory+"/flak_cannon.json",flak_cannonFields()));
    bounce_balance::Cone=bounce_balance::ReferenceCone*bounce_balance::SpreadMultiplier;
    return report;
}
} // namespace weapon_config

