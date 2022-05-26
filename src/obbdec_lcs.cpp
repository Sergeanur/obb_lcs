#include "WadFile.h"

int wmain(int argc, wchar_t* argv[])
{
	printf("obbdec_lcs v2.0 by Serge (aka Sergeanur)\n");
	if (argc < 2)
	{
		printf("No input file specified.\nYou must specify an OBB file to be unpacked.\n");
		return 0;
	}

	std::filesystem::path path = argv[1];
	cWadFile wad = cWadFile::Open(path);
	if (wad.IsOpen())
	{
		dictionary dict = ReadDictionary("dictionary.txt");
		wad.FillNamesFromDictionary(dict);
		//wad.PrintAllFiles();

		path.replace_extension();
		wad.ExtractAllFiles(path);

		//DumpUsedDictionary(dict, "Used_dictionary.txt");
	}
	else
	{
		printf("File %s could not be opened\n", path.string().c_str());
	}
	return 0;
}