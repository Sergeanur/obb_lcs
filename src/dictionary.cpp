#include "dictionary.h"
#include "crc32.h"
#include "utils.h"

dictionary_string dictionary_string::GetName() const
{
	size_t slash = find_last_of('/');
	if (slash == npos)
		return *this;
	return dictionary_string(std::move(substr(slash + 1)));
}

dictionary_string dictionary_string::GetPath() const
{
	size_t slash = find_last_of('/');
	if (slash == npos)
		return "";
	return dictionary_string(std::move(substr(0, slash)));
}

dictionary ReadDictionary(std::filesystem::path path)
{
	dictionary dict;

	if (!std::filesystem::exists(path))
	{
		if (path.is_absolute())
			return dict;

		path = GetApplicationPath() / path;
		if (!std::filesystem::exists(path))
			return dict;
	}

	FILE* ftxt = nullptr;
	_wfopen_s(&ftxt, path.c_str(), L"r");
	if (ftxt)
	{
		fseek(ftxt, 0, SEEK_END);
		size_t filesize = ftell(ftxt);
		fseek(ftxt, 0, SEEK_SET);

		std::vector<char> bufFile(filesize+1, '\0');
		filesize = fread(bufFile.data(), 1, filesize, ftxt);
		bufFile.resize(filesize+1, '\0');

		size_t start = 0;

		for (size_t i = 0; i < filesize; i++)
		{
			if ((bufFile[i] == '\n') || (bufFile[i] == '\r'))
				bufFile[i] = '\0';

			if (bufFile[i] == '\0' || i == filesize-1)
			{
				if (start != i)
					dict.emplace(crc32FromLowcaseString(&bufFile[start]), &bufFile[start]);
				start = i + 1;
			}
		}

		fclose(ftxt);
	}

	return dict;
}

void DumpUsedDictionary(const dictionary& dict, const std::filesystem::path& path)
{
	FILE* ftxt = nullptr;
	_wfopen_s(&ftxt, path.c_str(), L"w");

	for (const auto& [crc32, str] : dict)
	{
		if (str.wasUsed)
			fprintf(ftxt, "%s\n", str.c_str());
	}

	fclose(ftxt);
}
