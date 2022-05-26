#pragma once

#include <filesystem>
#include <vector>
#include "dictionary.h"

class cWadFile
{
	/*==types==*/

	struct sHeader
	{
		uint32_t lwad;
		uint32_t u1, u2;
	};

	struct sDirEntry
	{
		int32_t parentDir = -1;
		uint32_t nameOffset = 0;

		sDirEntry() = default;
		sDirEntry(int32_t parent) : parentDir(parent) { };
	};

	struct sFileEntry
	{
		uint32_t crc32 = 0; // lowercase hash of file path
		uint32_t nameOffset = 0;
		uint32_t directoryId = 0;
		uint32_t data_crc32 = 0x61EA4E00; // xor 0x61EA4E00
		uint64_t offset = 0;
		uint64_t zsize = 0; // Was this meant for compressed size? Not sure why I added z prefix. Both size fields are equal anyway.
		uint64_t size = 0;
		uint64_t _unused = 0;

		// for std::sort
		bool operator<(sFileEntry& r)
		{
			return crc32 < r.crc32;
		}
	};


	/*==fields==*/

	std::vector<sDirEntry> m_directories;
	std::vector<sFileEntry> m_files;

	std::vector<char> m_names; // list of names for dirs and files

	std::filesystem::path m_filepath;
	FILE* m_pFile = nullptr;

	bool m_bEncrypted = false;
	bool m_bHasNames = false;
	bool m_bPacked = false;


	/*==methods==*/

private:
	cWadFile() = default;
	cWadFile(const cWadFile&) = delete;
	cWadFile(cWadFile&& other); // this is here because RVO didn't happen, bruh...
	static void DoEncryption(void* _buf, size_t nSize, long long nOffset = 0);


	/* internal stuff for names */

	uint32_t InsertName(const char* name);
	void SetNameAtOffset(const char* str, size_t offset);
	void SetNameForDir(size_t dir, const dictionary_string& name);


	/* path getters */

	std::filesystem::path GetFilePath(const sFileEntry& file) const;
	std::filesystem::path GetDirPath(const sDirEntry& dir) const;
public:
	~cWadFile();
	inline bool IsOpen() const { return m_pFile != nullptr; }


	/* file IO functions */

	long long FileTell();
	bool FileSeek(long long pos);
	size_t FileRead(void* dest, size_t count);
	size_t FileWrite(const void* src, size_t count);
	void FileAlign(size_t alignment);

	template <class T>
	size_t FileReadVector(std::vector<T>& vec)
	{
		uint32_t count = 0;
		size_t bytesRead = FileRead(&count, sizeof(count));
		vec.resize(count);
		bytesRead += FileRead(vec.data(), count * sizeof(T));
		return bytesRead;
	}

	template <class T>
	size_t FileWriteVector(const std::vector<T>& vec)
	{
		uint32_t count = vec.size();
		size_t bytesWritten = FileWrite(&count, sizeof(count));
		bytesWritten += FileWrite(vec.data(), count * sizeof(T));
		return bytesWritten;
	}


	/* path getters (id) */

	inline std::filesystem::path GetFilePath(size_t file) const { return GetFilePath(m_files[file]); };
	inline std::filesystem::path GetDirPath(size_t dir) const { return GetDirPath(m_directories[dir]); }


	/* stuff for packing */

	bool ImportDirectory(const std::filesystem::path& path);
	bool Pack(const std::filesystem::path& path);


	/* stuff for unpacking */

	void FillNamesFromDictionary(const dictionary& dict);
	bool ExtractAllFiles(const std::filesystem::path& path);


	/* cWadFile object makers */

	// open file for reading (unpacking)
	static cWadFile Open(const std::filesystem::path& path);
	// create file for packing
	static cWadFile Create(const std::filesystem::path& path, bool isEncrypted);


	/* misc */

	void SaveListOfFilesToTXT(const std::filesystem::path& path) const;
	void PrintAllFiles() const;
	void DumpFiles() const;
};