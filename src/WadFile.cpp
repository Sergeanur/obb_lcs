#include "WadFile.h"
#include "crc32.h"
#include <functional>

const size_t BLOCK_SIZE = 0x20000; // size of file io buffer


cWadFile::cWadFile(cWadFile&& other)
{
	std::swap(m_directories, other.m_directories);
	std::swap(m_files, other.m_files);
	std::swap(m_names, other.m_names);
	std::swap(m_filepath, other.m_filepath);
	m_pFile = other.m_pFile;
	other.m_pFile = nullptr;
	m_bEncrypted = other.m_bEncrypted;
	m_bHasNames = other.m_bHasNames;
	m_bPacked = other.m_bPacked;
}

cWadFile::~cWadFile()
{
	if (m_pFile)
		fclose(m_pFile);
}



// file IO functions

long long cWadFile::FileTell()
{
	if (m_pFile)
		return _ftelli64(m_pFile);
	return -1ll;
}

bool cWadFile::FileSeek(long long pos)
{
	if (m_pFile)
		return _fseeki64(m_pFile, pos, SEEK_SET) == 0;
	return false;
}

size_t cWadFile::FileRead(void* dest, size_t count)
{
	if (!m_bPacked) return 0;
	if (count == 0) return 0;
	if (!m_pFile)  return 0;

	long long curPos = FileTell();
	size_t r = fread(dest, 1, count, m_pFile);
	if (m_bEncrypted && r > 0)
		DoEncryption(dest, r, curPos);
	return r;
}

size_t cWadFile::FileWrite(const void* src, size_t count)
{
	if (m_bPacked) return 0;
	if (count == 0) return 0;
	if (!m_pFile) return 0;

	if (m_bEncrypted)
	{
		static std::vector<uint8_t> tempBuf;
		if (tempBuf.size() < count)
			tempBuf.resize(count);
		memcpy(tempBuf.data(), src, count);
		DoEncryption(tempBuf.data(), count, FileTell());
		return fwrite(tempBuf.data(), 1, count, m_pFile);
	}
	else
		return fwrite(src, 1, count, m_pFile);

}

void cWadFile::FileAlign(size_t alignment)
{
	long long pos = FileTell();
	if (pos % alignment)
	{
		if (m_bPacked)
		{
			FileSeek(pos + (alignment - (pos % alignment)));
		}
		else
		{
			// create buffer to fill in
			std::vector<uint8_t> padding(alignment - (pos % alignment), 0);
			FileWrite(padding.data(), padding.size());
		}
	}
}



// names related functions

uint32_t cWadFile::InsertName(const char* name)
{
	uint32_t result = m_names.size();

	// find if this name was already inserted
	for (uint32_t i = 0; i < result; i++)
	{
		if (!strcmp(name, &m_names[i]))
			return i;
		i += strlen(&m_names[i]);
	}

	// insert if not found
	do
		m_names.push_back(*name);
	while (*(name++) != '\0');

	return result;
}

void cWadFile::SetNameAtOffset(const char* str, size_t offset)
{
	size_t len = strlen(str) + 1;
	if (m_names.size() < len + offset)
		m_names.resize(len + offset);

	char* name = &m_names[offset];
	if (*name == '\0')
		strcpy_s(name, len, str);
	else
	{
		if (strcmp(name, str))
		{
			printf("ERROR: Tried emplacing %s over %s in the names vector\n", str, name);
			throw 0;
		}
	}
}

void cWadFile::SetNameForDir(size_t dir, const dictionary_string& name)
{
	dictionary_string filename = name.GetName();
	dictionary_string path = name.GetPath();

	SetNameAtOffset(filename.c_str(), m_directories[dir].nameOffset);
	if (m_directories[dir].parentDir != -1)
		SetNameForDir(m_directories[dir].parentDir, path);
}

void cWadFile::FillNamesFromDictionary(const dictionary& dict)
{
	if (m_bHasNames) return;
	for (const auto& file : m_files)
	{
		auto it = dict.find(file.crc32);
		if (it != dict.end())
		{
			it->second.wasUsed = true;
			dictionary_string filename = it->second.GetName();
			dictionary_string path = it->second.GetPath();

			//printf("%s %s\n", path.c_str(), filename.c_str());

			SetNameAtOffset(filename.c_str(), file.nameOffset);
			SetNameForDir(file.directoryId, path);
		}
		else
		{
			printf("NOT FOUND %x\n", file.crc32);
		}
	}
	m_bHasNames = true;
}



// path getters

std::filesystem::path cWadFile::GetFilePath(const sFileEntry& file) const
{
	if (m_names[file.nameOffset] == '\0')
	{
		char crc_str[24];
		sprintf_s(crc_str, "%08X.dat", file.crc32);
		return crc_str;
	}
	return GetDirPath(file.directoryId) / &m_names[file.nameOffset];
}

std::filesystem::path cWadFile::GetDirPath(const sDirEntry& dir) const
{
	if (dir.parentDir == -1)
		return &m_names[dir.nameOffset];
	return GetDirPath(m_directories[dir.parentDir]) / &m_names[dir.nameOffset];
}



// misc stuff

void cWadFile::SaveListOfFilesToTXT(const std::filesystem::path& path) const
{
	FILE* txt = nullptr;
	_wfopen_s(&txt, path.c_str(), L"w");
	if (!txt) return;

	for (const auto& file : m_files)
		fprintf(txt, "%s\n", GetFilePath(file).string().c_str());

	fclose(txt);
}

void cWadFile::PrintAllFiles() const
{
	for (const auto& file : m_files)
		printf("%s\n", GetFilePath(file).string().c_str());
}

void cWadFile::DumpFiles() const
{
	FILE* f = nullptr;
	fopen_s(&f, "filedump.bin", "wb");
	if (!f) return;

	fwrite(m_files.data(), sizeof(sFileEntry), m_files.size(), f);

	fclose(f);
}



// stuff for packing

bool cWadFile::ImportDirectory(const std::filesystem::path& path)
{
	if (m_bPacked)
		return false;

	if (!std::filesystem::exists(path))
		return false;

	std::vector<std::string> dirs;

	std::function<void(const std::filesystem::path&, int32_t)> ProcessDir = 
	[this, &dirs, &srcpath = path, &ProcessDir](const std::filesystem::path& path, int32_t parentId)
	{
		// Save the lists of files and directories to then sort them
		// This is not required but produces more accurate results
		std::vector<std::string> here_files;
		std::vector<std::string> here_dirs;

		for (auto entry : std::filesystem::directory_iterator(path))
		{
			if (!entry.is_directory())
				here_files.emplace_back(entry.path().filename().string());
			else
				here_dirs.emplace_back(entry.path().filename().string());
		}

		if (here_files.size() == 0 && here_dirs.size() == 0) // check if dir is empty
			return;

		std::sort(here_files.begin(), here_files.end());
		std::sort(here_dirs.begin(), here_dirs.end());

		if (srcpath == path)
			dirs.emplace_back("");
		else
			dirs.emplace_back(path.stem().string());
		m_directories.emplace_back(parentId);

		// Add files into file vector
		int32_t currentDirId = dirs.size() - 1;
		for (const auto& file : here_files)
		{
			sFileEntry entry;
			entry.nameOffset = InsertName(file.c_str());
			entry.directoryId = currentDirId;
			m_files.push_back(entry);
		}


		// process the other directories recursively
		for (const auto& dir : here_dirs)
			ProcessDir(path / dir, currentDirId);
	};

	printf("Creating a list of files to process... ");
	ProcessDir(path, -1);
	printf("done!\n");

	for (size_t i = 0; i < dirs.size(); i++)
		m_directories[i].nameOffset = InsertName(dirs[i].c_str());

	for (auto& file : m_files)
	{
		std::string filename = GetFilePath(file).string();

		// convert backslash to forward slash
		for (char& c : filename)
		{
			if (c == '\\')
				c = '/';
		}

		file.crc32 = crc32FromLowcaseString(filename.c_str());
	}

	//PrintAllFiles();

	return true;
}

bool cWadFile::Pack(const std::filesystem::path& path)
{
	if (m_bPacked)
		return false;
	static std::vector<uint8_t> tempBuf(BLOCK_SIZE);

	auto WriteHeader = [this]()
	{
		FileSeek(0x800);

		FileWriteVector(m_directories);
		FileWriteVector(m_files);
	};

	WriteHeader();

	FileAlign(0x800);

	for (auto& file : m_files)
	{
		std::filesystem::path infilePath = path / GetFilePath(file);

		FILE* infile = nullptr;
		_wfopen_s(&infile, infilePath.c_str(), L"rb");
		if (!infile)
		{
			printf("Couldn't open file %s\n", infilePath.string().c_str());
			continue;
		}

		file.offset = FileTell();

		_fseeki64(infile, 0, SEEK_END);
		uint64_t size = file.zsize = file.size = _ftelli64(infile);
		_fseeki64(infile, 0, SEEK_SET);
		printf("%s\n", infilePath.lexically_relative(path).string().c_str());

		file.data_crc32 = 0;
		while (size > 0)
		{
			size_t sizeOfThisBlock = (size < BLOCK_SIZE) ? static_cast<size_t>(size) : BLOCK_SIZE;
			fread(tempBuf.data(), 1, sizeOfThisBlock, infile);
			file.data_crc32 = crc32wAppend(tempBuf.data(), sizeOfThisBlock, file.data_crc32);
			FileWrite(tempBuf.data(), sizeOfThisBlock);
			size -= sizeOfThisBlock;
		}

		fclose(infile);

		file.data_crc32 ^= 0x61EA4E00;
	}

	std::sort(m_files.begin(), m_files.end());


	// rewrite updated header
	WriteHeader();
	
	m_bPacked = true;

	// file is packed, make it read only
	_wfreopen_s(&m_pFile, m_filepath.c_str(), L"rb", m_pFile);

	return true;
}



// stuff for unpacking

bool cWadFile::ExtractAllFiles(const std::filesystem::path& path)
{
	if (!m_bPacked)
		return false;
	static std::vector<uint8_t> tempBuf(BLOCK_SIZE);

	for (const auto& dir : m_directories)
	{
		auto d = path / GetDirPath(dir);
		std::filesystem::create_directories(d);
	}

	for (const auto& file : m_files)
	{
		FileSeek(file.offset);
		std::filesystem::path filepath = path / GetFilePath(file);

		FILE* outfile = nullptr;
		_wfopen_s(&outfile, filepath.c_str(), L"wb");
		if (!outfile)
		{
			printf("Couldn't create file %s\n", filepath.string().c_str());
			continue;
		}

		uint64_t size = file.size;
		printf("%s\n", filepath.lexically_relative(path).string().c_str());
		while (size > 0)
		{
			size_t sizeOfThisBlock = (size < BLOCK_SIZE) ? static_cast<size_t>(size) : BLOCK_SIZE;
			FileRead(tempBuf.data(), sizeOfThisBlock);
			fwrite(tempBuf.data(), 1, sizeOfThisBlock, outfile);
			size -= sizeOfThisBlock;
		}

		fclose(outfile);
	}

	return true;
}



// static funcs

void cWadFile::DoEncryption(void* _buf, size_t nSize, long long nOffset)
{
	if (nSize < 1) return;
	uint8_t* buf = (uint8_t*)_buf;

#if 1
	// this works a lot faster
	if (nOffset & 1)
	{
		*(buf++) ^= 0x66;
		nSize--;
	}

	while (nSize >= sizeof(uint32_t))
	{
		*(uint32_t*)buf ^= 0x66AF66AF;
		buf += sizeof(uint32_t);
		nSize -= sizeof(uint32_t);
	}
	if (nSize >= sizeof(uint16_t))
	{
		*(uint16_t*)buf ^= 0x66AF;
		buf += sizeof(uint16_t);
		nSize -= sizeof(uint16_t);
	}
	if (nSize >= sizeof(uint8_t))
		*buf ^= 0xAF;
#else
	// short version of what's going on
	for (long long i = nOffset; i < (nSize + nOffset); i++)
		*(buf++) ^= (i & 1) ? 0x66 : 0xAF;
#endif
}

cWadFile cWadFile::Open(const std::filesystem::path& path)
{
	cWadFile wadFile;

	_wfopen_s(&wadFile.m_pFile, path.c_str(), L"rb");

	if (wadFile.m_pFile)
	{
		sHeader header;
		fread(&header, sizeof(header), 1, wadFile.m_pFile);

		if (header.lwad == 'LWAD') wadFile.m_bEncrypted = false;
		else
		{
			DoEncryption(&header, sizeof(header));
			if (header.lwad == 'LWAD')
				wadFile.m_bEncrypted = true;
			else
			{
				fclose(wadFile.m_pFile);
				wadFile.m_pFile = nullptr;
				return wadFile;
			}
		}
		wadFile.m_filepath = path;
		wadFile.m_bPacked = true;

		wadFile.FileAlign(0x800);

		wadFile.FileReadVector(wadFile.m_directories);
		wadFile.FileReadVector(wadFile.m_files);

		uint32_t maxNameOffset = 0;
		for (const auto& i : wadFile.m_directories)
		{
			if (i.nameOffset > maxNameOffset)
				maxNameOffset = i.nameOffset;
		}
		for (const auto& i : wadFile.m_files)
		{
			if (i.nameOffset > maxNameOffset)
				maxNameOffset = i.nameOffset;
		}

		if (maxNameOffset > 0)
			wadFile.m_names.resize(maxNameOffset, '\0');
	}

	return wadFile;
}

cWadFile cWadFile::Create(const std::filesystem::path& path, bool isEncrypted)
{
	cWadFile wadFile;

	_wfopen_s(&wadFile.m_pFile, path.c_str(), L"wb");
	if (wadFile.m_pFile)
	{
		wadFile.m_filepath = path;
		wadFile.m_bEncrypted = isEncrypted;
		wadFile.m_bHasNames = true;

		sHeader header{ 'LWAD', 1, 1 };
		wadFile.FileWrite(&header, sizeof(header));

		wadFile.FileAlign(0x800);
	}

	return wadFile;
}
