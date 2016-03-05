#ifndef DISK_LIST
#define DISK_LIST

#include <string>

#include "BinaryFile.h"

class DiskList
{
public:
	DiskList(const std::string& filename);
	bool push_front(const char* data);
	bool remove(const char* data);
	void printAll();

private:
	BinaryFile m_file;
	int unusedBytes;
};

#pragma once

#endif