#pragma once
#include <vector>
#include <string>
#include <sstream>

namespace stc::tools
{
	/*
	*	Split string using _Split param as delimiter. Single space is used by default.
	*/
	std::vector<std::string> SplitString(const std::string& _InStr, const char& _Split = ' ')
	{
		std::vector<std::string> Tokens;
		std::istringstream iss(_InStr);
		std::string s;

		while (getline(iss, s, ' ')) 
		{
			Tokens.push_back(s);
		}

		return Tokens;
	}
}