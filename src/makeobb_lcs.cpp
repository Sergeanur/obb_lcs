#include "WadFile.h"

int wmain(int argc, wchar_t* argv[])
{
	printf("makeobb_lcs v2.0 by Serge (aka Sergeanur)\n");
	if (argc < 2)
	{
		printf("No input directory specified.\nYou must specify a directory that would be packaged into OBB.\n");
		return 0;
	}

	std::filesystem::path path = argv[1];
	path.replace_filename(path.filename().wstring() + L".obb");

	cWadFile wad = cWadFile::Create(path, true);
	if (wad.IsOpen())
	{
		wad.ImportDirectory(argv[1]);
		wad.Pack(argv[1]);
		path.replace_extension();
		path.replace_filename(path.filename().wstring() + L"_filelist.txt");
		wad.SaveListOfFilesToTXT(path);
	}
	else
	{
		printf("File %s could not be created\n", path.string().c_str());
	}
	return 0;
}