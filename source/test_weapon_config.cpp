#include <cassert>
#include <cstdio>
#include "weapon_config.h"
int main(){
 using namespace weapon_config;
 std::string error;
 auto ric=ricochetFields(),flak=flak_cannonFields();
 assert(apply(R"({"max_charges":5,"recharge_seconds":3.5})",ric,error));
 assert(balance::MaxCharges==5 && balance::RechargeTime==3.5f);
 assert(!apply(R"({"max_charges":8,"recharge_seconds":0})",ric,error));
 assert(balance::MaxCharges==5 && balance::RechargeTime==3.5f); // atomic rejection
 for(auto text:{R"({"max_charges":1.5})",R"({"max_charges":true})",R"({"max_charges":3,"max_charges":4})",R"({"recharge_seconds":NaN})",R"({"recharge_seconds":1e999})",R"({"max_charges":03})",R"({"typo":1})",R"({"max_charges ":3})",R"({"max_charges":3,})",R"({"max_charges":3} garbage)"})assert(!apply(text,ric,error));
 assert(apply(R"({"primary_ammo_cost":3,"secondary_ammo_cost":4,"pellets_per_shot":8,"gib_on_lethal_overkill":false})",flak,error));
 assert(bounce_balance::PrimaryAmmoCost==3 && bounce_balance::SecondaryAmmoCost==4 && bounce_balance::Pellets==8 && !bounce_balance::OverkillGib);
 assert(!apply(R"({"gib_on_lethal_overkill":1})",flak,error));
 assert(!apply(R"({"pellets_per_shot":1000})",flak,error));
 assert(load("file-that-does-not-exist.json",ric).find("retained")!=std::string::npos);
 // Load installed JSONs: proves real disk paths preserve the current balance.
 auto report=loadAll("../config/weapons");
 for(auto& line:report){std::puts(line.c_str());assert(line.find(": loaded")!=std::string::npos);}
 for(auto name:{"ricochet","flak_cannon"}) {
   std::ifstream file(std::string("../config/weapons/")+name+".json");
   std::string data((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
   std::map<std::string,Value> values;assert(Reader{data}.object(values));
   auto fields=std::string(name)=="ricochet"?ric:flak;
   for(auto& f:fields)if(values.count(f.key))assert(std::fabs(f.read()-values[f.key].number)<.0001);
 }
 assert(std::fabs(bounce_balance::Cone-bounce_balance::ReferenceCone*bounce_balance::SpreadMultiplier)<.00001f);
 std::puts("PASS: JSON disk loading, numeric/boolean types, ranges, duplicates, syntax, unknown keys, atomic rejection and current defaults.");
}
