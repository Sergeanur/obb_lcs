#include <filesystem>
#include <vector>
#include <Windows.h>
#include "utils.h"

const std::filesystem::path& GetApplicationPath()
{
	static std::filesystem::path path;

	if (path.empty())
	{
		std::vector<WCHAR> name(MAX_PATH);
		DWORD errorCode = 0;
		do
		{
			DWORD result = GetModuleFileName(NULL, name.data(), name.size());
			errorCode = GetLastError();
			switch (errorCode)
			{
			case ERROR_SUCCESS:
				path = name.data();
				path.remove_filename();
				break;
			case ERROR_INSUFFICIENT_BUFFER:
				//printf("ERROR_INSUFFICIENT_BUFFER\n");
				name.resize(name.size() + MAX_PATH);
				break;
			default:
				printf("GetApplicationPath() unknown error %X\n", errorCode);
				break;
			}
		} while (errorCode == ERROR_INSUFFICIENT_BUFFER);
	}

	return path;
}