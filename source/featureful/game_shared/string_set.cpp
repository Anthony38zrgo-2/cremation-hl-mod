#include "string_set.h"

const char* MakeStringInSet(std::set<std::string>& set, const char* cstr)
{
	std::string str = cstr;
	auto strIt = set.find(str);
	if (strIt == set.end())
	{
		auto p = set.insert(str);
		strIt = p.first;
	}
	return strIt->c_str();
}
