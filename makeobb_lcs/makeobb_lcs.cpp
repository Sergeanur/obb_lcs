#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

struct DirEntry
{
	int parentDir;
	DWORD nameOffset;

	DirEntry(int a1) : parentDir(a1)
	{ };
};

struct FileEntry
{
	DWORD crc32;
	DWORD filenameOffset, folderId;
	DWORD data_crc32; // xor 0x61EA4E00
	unsigned __int64 offset, zsize, size, u4;

	FileEntry(DWORD _crc32, DWORD _folderId)
	{
		memset(this, 0, sizeof(FileEntry));
		this->crc32 = _crc32;//crc32FromLowcaseString(path);
		folderId = _folderId;
	}

	bool operator<(FileEntry& r)
	{
		return crc32 < r.crc32;
	}
};


static const DWORD crcTable[256] = {
    0x00000000UL, 0x77073096UL, 0xee0e612cUL, 0x990951baUL, 0x076dc419UL,
    0x706af48fUL, 0xe963a535UL, 0x9e6495a3UL, 0x0edb8832UL, 0x79dcb8a4UL,
    0xe0d5e91eUL, 0x97d2d988UL, 0x09b64c2bUL, 0x7eb17cbdUL, 0xe7b82d07UL,
    0x90bf1d91UL, 0x1db71064UL, 0x6ab020f2UL, 0xf3b97148UL, 0x84be41deUL,
    0x1adad47dUL, 0x6ddde4ebUL, 0xf4d4b551UL, 0x83d385c7UL, 0x136c9856UL,
    0x646ba8c0UL, 0xfd62f97aUL, 0x8a65c9ecUL, 0x14015c4fUL, 0x63066cd9UL,
    0xfa0f3d63UL, 0x8d080df5UL, 0x3b6e20c8UL, 0x4c69105eUL, 0xd56041e4UL,
    0xa2677172UL, 0x3c03e4d1UL, 0x4b04d447UL, 0xd20d85fdUL, 0xa50ab56bUL,
    0x35b5a8faUL, 0x42b2986cUL, 0xdbbbc9d6UL, 0xacbcf940UL, 0x32d86ce3UL,
    0x45df5c75UL, 0xdcd60dcfUL, 0xabd13d59UL, 0x26d930acUL, 0x51de003aUL,
    0xc8d75180UL, 0xbfd06116UL, 0x21b4f4b5UL, 0x56b3c423UL, 0xcfba9599UL,
    0xb8bda50fUL, 0x2802b89eUL, 0x5f058808UL, 0xc60cd9b2UL, 0xb10be924UL,
    0x2f6f7c87UL, 0x58684c11UL, 0xc1611dabUL, 0xb6662d3dUL, 0x76dc4190UL,
    0x01db7106UL, 0x98d220bcUL, 0xefd5102aUL, 0x71b18589UL, 0x06b6b51fUL,
    0x9fbfe4a5UL, 0xe8b8d433UL, 0x7807c9a2UL, 0x0f00f934UL, 0x9609a88eUL,
    0xe10e9818UL, 0x7f6a0dbbUL, 0x086d3d2dUL, 0x91646c97UL, 0xe6635c01UL,
    0x6b6b51f4UL, 0x1c6c6162UL, 0x856530d8UL, 0xf262004eUL, 0x6c0695edUL,
    0x1b01a57bUL, 0x8208f4c1UL, 0xf50fc457UL, 0x65b0d9c6UL, 0x12b7e950UL,
    0x8bbeb8eaUL, 0xfcb9887cUL, 0x62dd1ddfUL, 0x15da2d49UL, 0x8cd37cf3UL,
    0xfbd44c65UL, 0x4db26158UL, 0x3ab551ceUL, 0xa3bc0074UL, 0xd4bb30e2UL,
    0x4adfa541UL, 0x3dd895d7UL, 0xa4d1c46dUL, 0xd3d6f4fbUL, 0x4369e96aUL,
    0x346ed9fcUL, 0xad678846UL, 0xda60b8d0UL, 0x44042d73UL, 0x33031de5UL,
    0xaa0a4c5fUL, 0xdd0d7cc9UL, 0x5005713cUL, 0x270241aaUL, 0xbe0b1010UL,
    0xc90c2086UL, 0x5768b525UL, 0x206f85b3UL, 0xb966d409UL, 0xce61e49fUL,
    0x5edef90eUL, 0x29d9c998UL, 0xb0d09822UL, 0xc7d7a8b4UL, 0x59b33d17UL,
    0x2eb40d81UL, 0xb7bd5c3bUL, 0xc0ba6cadUL, 0xedb88320UL, 0x9abfb3b6UL,
    0x03b6e20cUL, 0x74b1d29aUL, 0xead54739UL, 0x9dd277afUL, 0x04db2615UL,
    0x73dc1683UL, 0xe3630b12UL, 0x94643b84UL, 0x0d6d6a3eUL, 0x7a6a5aa8UL,
    0xe40ecf0bUL, 0x9309ff9dUL, 0x0a00ae27UL, 0x7d079eb1UL, 0xf00f9344UL,
    0x8708a3d2UL, 0x1e01f268UL, 0x6906c2feUL, 0xf762575dUL, 0x806567cbUL,
    0x196c3671UL, 0x6e6b06e7UL, 0xfed41b76UL, 0x89d32be0UL, 0x10da7a5aUL,
    0x67dd4accUL, 0xf9b9df6fUL, 0x8ebeeff9UL, 0x17b7be43UL, 0x60b08ed5UL,
    0xd6d6a3e8UL, 0xa1d1937eUL, 0x38d8c2c4UL, 0x4fdff252UL, 0xd1bb67f1UL,
    0xa6bc5767UL, 0x3fb506ddUL, 0x48b2364bUL, 0xd80d2bdaUL, 0xaf0a1b4cUL,
    0x36034af6UL, 0x41047a60UL, 0xdf60efc3UL, 0xa867df55UL, 0x316e8eefUL,
    0x4669be79UL, 0xcb61b38cUL, 0xbc66831aUL, 0x256fd2a0UL, 0x5268e236UL,
    0xcc0c7795UL, 0xbb0b4703UL, 0x220216b9UL, 0x5505262fUL, 0xc5ba3bbeUL,
    0xb2bd0b28UL, 0x2bb45a92UL, 0x5cb36a04UL, 0xc2d7ffa7UL, 0xb5d0cf31UL,
    0x2cd99e8bUL, 0x5bdeae1dUL, 0x9b64c2b0UL, 0xec63f226UL, 0x756aa39cUL,
    0x026d930aUL, 0x9c0906a9UL, 0xeb0e363fUL, 0x72076785UL, 0x05005713UL,
    0x95bf4a82UL, 0xe2b87a14UL, 0x7bb12baeUL, 0x0cb61b38UL, 0x92d28e9bUL,
    0xe5d5be0dUL, 0x7cdcefb7UL, 0x0bdbdf21UL, 0x86d3d2d4UL, 0xf1d4e242UL,
    0x68ddb3f8UL, 0x1fda836eUL, 0x81be16cdUL, 0xf6b9265bUL, 0x6fb077e1UL,
    0x18b74777UL, 0x88085ae6UL, 0xff0f6a70UL, 0x66063bcaUL, 0x11010b5cUL,
    0x8f659effUL, 0xf862ae69UL, 0x616bffd3UL, 0x166ccf45UL, 0xa00ae278UL,
    0xd70dd2eeUL, 0x4e048354UL, 0x3903b3c2UL, 0xa7672661UL, 0xd06016f7UL,
    0x4969474dUL, 0x3e6e77dbUL, 0xaed16a4aUL, 0xd9d65adcUL, 0x40df0b66UL,
    0x37d83bf0UL, 0xa9bcae53UL, 0xdebb9ec5UL, 0x47b2cf7fUL, 0x30b5ffe9UL,
    0xbdbdf21cUL, 0xcabac28aUL, 0x53b39330UL, 0x24b4a3a6UL, 0xbad03605UL,
    0xcdd70693UL, 0x54de5729UL, 0x23d967bfUL, 0xb3667a2eUL, 0xc4614ab8UL,
    0x5d681b02UL, 0x2a6f2b94UL, 0xb40bbe37UL, 0xc30c8ea1UL, 0x5a05df1bUL,
    0x2d02ef8dUL
};

DWORD crc32FromLowcaseString (char * key) {
	DWORD crc = 0xFFFFFFFF;
	while (*key) 
		crc = crcTable[(crc^tolower(*key++))&0xff]^(crc >> 8);
	return crc;
}

DWORD crc32wAppend (BYTE * key, size_t size, DWORD append = 0) {
	DWORD crc = append;
	while (size)
	{
		crc = crcTable[(crc ^ *(key++)) & 0xff] ^ (crc >> 8);
		size--;
	}
	return crc;
}

unsigned __int64 fsize(FILE *f)
{
	unsigned __int64 retpos = _ftelli64(f);
    _fseeki64(f, 0, SEEK_END);
    unsigned __int64 len = (unsigned __int64)_ftelli64(f);
	_fseeki64(f, retpos, SEEK_SET);
    return len;
}

struct Folder
{
	string name;
	int parent;

	Folder(char* str, int fid)
	{
		name = str;
		parent = fid;
	}

	bool operator<(Folder& f)
	{
		return name < f.name;
	}
};

struct FileS
{
	string name;
	string path;
	int directory;
	DWORD crc32;

	FileS(char* str, char* str_p, int fid, DWORD c)
	{
		name = str;
		path = str_p;
		directory = fid;
		crc32 = c;
	}

	bool operator<(FileS& f)
	{
		return name < f.name;
	}
};

vector<Folder> aFolders;
vector<FileS> aFiles;

char workdir[MAX_PATH] = "";

void FindFoldersAndFiles(const char* folder, int parentFolder = 0)
{
	WIN32_FIND_DATAA ffd;
	char filter[300];
	sprintf(filter, "%s%s*", workdir, folder); 
	HANDLE hFind = FindFirstFileA(filter, &ffd);
	vector<Folder> aCurFolders;
	auto beg = aFiles.size();
	do
	{
		if ((strcmp(ffd.cFileName, ".")) && (strcmp(ffd.cFileName, "..")))
		{
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				//printf("%s%s\n", folder, ffd.cFileName);
				//char filter2[300];
				//sprintf(filter2, "%s%s\\", folder, ffd.cFileName); 
				aCurFolders.push_back(Folder(ffd.cFileName, parentFolder));
				//FindFolders(filter2, aFolders.size() - 1);
			}
			else
			{
				char filepath2[300];
				sprintf(filepath2, "%s%s", folder, ffd.cFileName); 
				char filepath[300];
				sprintf(filepath, "%s%s", workdir, filepath2); 
				aFiles.push_back(FileS(ffd.cFileName, filepath, aFolders.size() - 1, crc32FromLowcaseString(filepath2)));
			}
		}
	}
	while (FindNextFileA(hFind, &ffd) != 0);
	auto end = aFiles.size();
	sort(&aFiles[beg], &aFiles[end]);
	
	sort(aCurFolders.begin(), aCurFolders.end());
	for (int i = 0; i < aCurFolders.size(); i++)
	{
		aFolders.push_back(aCurFolders[i]);
		char filter2[300];
		sprintf(filter2, "%s%s/", folder, aCurFolders[i].name.c_str()); 
		FindFoldersAndFiles(filter2, aFolders.size() - 1);
	}
}

void decrypt(BYTE* buf, size_t nSize, DWORD dwOffset = 0)
{
	for (DWORD i = dwOffset; i < (nSize + dwOffset); i++)
		*(buf++) ^= (i & 1) ? 0x66 : 0xAF;
}

bool bEncrypted = true;

size_t fwrite_enc2(void* a1, size_t a2, size_t a3, FILE* a4)
{
	auto cur_pos = _ftelli64(a4);
	BYTE* buf = new BYTE [a2 * a3];
	decrypt((BYTE*)a1, a2 * a3, cur_pos);
	return fwrite(a1, a2, a3, a4);
}

size_t fwrite_enc(void* a1, size_t a2, size_t a3, FILE* a4)
{
	auto cur_pos = _ftelli64(a4);
	if (bEncrypted)
	{
		BYTE* buf = new BYTE [a2 * a3];
		memcpy(buf, a1, a2 * a3);
		decrypt(buf, a2 * a3, cur_pos);
		auto result = fwrite(buf, a2, a3, a4);
		delete []buf;
		return result;
	}
	else return fwrite(a1, a2, a3, a4);
}

#define _BLOCK_SIZE 0x2000

DWORD BlockReadWrite(FILE* in, FILE* out, int _Size)
{
	BYTE fbuf[_BLOCK_SIZE];

	while (_Size >= 0)
	{
		_Size -= _BLOCK_SIZE;
		size_t BLOCK_SIZE = _BLOCK_SIZE;
		if (_Size < 0) BLOCK_SIZE = _BLOCK_SIZE + _Size;
		fread(fbuf, 1, BLOCK_SIZE, in);
		fwrite_enc2(fbuf, 1, BLOCK_SIZE, out);
	}
}

DWORD BlockReadWriteCRC32(FILE* in, FILE* out, int _Size)
{
	BYTE fbuf[_BLOCK_SIZE];

	DWORD crc32 = 0;
	while (_Size >= 0)
	{
		_Size -= _BLOCK_SIZE;
		size_t BLOCK_SIZE = _BLOCK_SIZE;
		if (_Size < 0) BLOCK_SIZE = _BLOCK_SIZE + _Size;
		fread(fbuf, 1, BLOCK_SIZE, in);
		crc32 = crc32wAppend(fbuf, BLOCK_SIZE, crc32);
		fwrite_enc2(fbuf, 1, BLOCK_SIZE, out);
	}
	return crc32;
}


vector<char> FilenameArray;
vector<DWORD> FilenameArrayOffsets;
DWORD GetFilenameOffset(char* str)
{
	for (int i = 0; i < FilenameArrayOffsets.size(); i++)
	{
		if (!strcmp(&FilenameArray.data()[FilenameArrayOffsets[i]], str))
			return FilenameArrayOffsets[i];
	}

	int result = FilenameArray.size();
	for (int i = 0; i < strlen(str); i++)
		FilenameArray.push_back(str[i]);
	FilenameArray.push_back(0);
	FilenameArrayOffsets.push_back(result);
	return result;
}

void main(int argc, char* argv[])
{
	if (argc < 2) return;
	//printf("%s\n", argv[1]);
	
	sprintf(workdir, "%s\\", argv[1]);
	printf("%s\n", workdir);

	aFolders.push_back(Folder("", -1));
	FindFoldersAndFiles("");

	//for (int i = 0; i < aFolders.size(); i++)
	//	printf("%s %i\n", aFolders[i].name.c_str(), aFolders[i].parent);

	//for (int i = 0; i < aFiles.size(); i++)
	//	printf("%s %i\n", aFiles[i].name.c_str(), aFiles[i].directory);
	//system("pause");

	char obbname[MAX_PATH];
	sprintf(obbname, "%s.obb", argv[1]);
	FILE* f = fopen(obbname, "wb");

	struct
	{
		DWORD lwad;
		DWORD u1, u2;
	} Header;

	Header.lwad = 'LWAD';
	Header.u1 = Header.u2 = 1;

	fwrite_enc(&Header, 12, 1, f);
	BYTE* padding_bs = new BYTE [0x800 - 12];
	memset(padding_bs, 0, 0x800 - 12);
	fwrite_enc(padding_bs, 0x800 - 12, 1, f);
	delete []padding_bs;

	//_fseeki64(f, 0x800, SEEK_SET);
	int DirSecSize = aFolders.size();
	fwrite_enc(&DirSecSize, 4, 1, f);

	vector <DirEntry> aDirs;
	for (int i = 0; i < DirSecSize; i++)
		aDirs.push_back(DirEntry(aFolders[i].parent));
	fwrite_enc(aDirs.data(), sizeof(DirEntry), DirSecSize, f);

	int FileSecSize = aFiles.size();
	vector <FileEntry> aFiles_e;
	for (int i = 0; i < FileSecSize; i++)
		aFiles_e.push_back(FileEntry(aFiles[i].crc32, aFiles[i].directory));
	fwrite_enc(&FileSecSize, 4, 1, f);
	fwrite_enc(aFiles_e.data(), sizeof(FileEntry), FileSecSize, f);

	if (auto mod_800 = _ftelli64(f) % 0x800)
	{	
		padding_bs = new BYTE [0x800 - mod_800];
		memset(padding_bs, 0, 0x800 - mod_800);
		fwrite_enc(padding_bs, 0x800 - mod_800, 1, f);
		delete []padding_bs;
		//_fseeki64(f, 0x800 - mod_800, SEEK_CUR);
	}

	for (int i = 0; i < aFiles.size(); i++)
	{
		aFiles_e[i].filenameOffset = GetFilenameOffset((char*)aFiles[i].name.c_str());
		aFiles_e[i].offset = _ftelli64(f);
		printf("%s\n", aFiles[i].path.c_str());
		FILE* infile = fopen(aFiles[i].path.c_str(), "rb");
		aFiles_e[i].zsize = aFiles_e[i].size = fsize(infile);
		aFiles_e[i].data_crc32 = BlockReadWriteCRC32(infile, f, aFiles_e[i].size) ^ 0x61EA4E00;
		fclose(infile);
	}

	for (int i = 0; i < DirSecSize; i++)
		aDirs[i].nameOffset = GetFilenameOffset((char*)aFolders[i].name.c_str());

	_fseeki64(f, 0x804, SEEK_SET);
	fwrite_enc(aDirs.data(), sizeof(DirEntry), DirSecSize, f);

	sort(aFiles_e.begin(), aFiles_e.end());
	_fseeki64(f, 4, SEEK_CUR);
	fwrite_enc(aFiles_e.data(), sizeof(FileEntry), FileSecSize, f);

	fclose(f);
}