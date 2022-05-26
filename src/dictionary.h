#pragma once

#include <string>
#include <map>
#include <filesystem>

class dictionary_string : public std::string
{
public:
	using std::string::string;

	mutable bool wasUsed = false;

	dictionary_string GetName() const;
	dictionary_string GetPath() const;
};

using dictionary = std::map<uint32_t, dictionary_string>;

dictionary ReadDictionary(std::filesystem::path path);
void DumpUsedDictionary(const dictionary& dict, const std::filesystem::path& path);
