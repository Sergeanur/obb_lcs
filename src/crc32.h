#pragma once

uint32_t crc32FromUpcaseString(const char* key);
uint32_t crc32FromLowcaseString(const char* key);
uint32_t crc32wAppend(void* data, size_t size, uint32_t crc = 0);