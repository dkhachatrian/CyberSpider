#include "DiskMultiMap.h"
#include "BinaryFile.h"
#include <functional>

DiskMultiMap::Iterator::Iterator()
{
	m_valid = false;
}

DiskMultiMap::Iterator::Iterator(BinaryFile::Offset startLoc, BinaryFile& hash)
{
	char data[MAX_NODE_SIZE + 1];
	if (hash.read(data, MAX_NODE_SIZE + 1, startLoc))
	{

	}
}

bool DiskMultiMap::Iterator::isValid() const
{
	return m_valid;
}
DiskMultiMap::Iterator& DiskMultiMap::Iterator::operator++()
{
	if (!isValid())
		return *this;
	//otherwise increment up the MultiMap
}
MultiMapTuple DiskMultiMap::Iterator::operator*()
{

}



DiskMultiMap::DiskMultiMap()
{
	
}

DiskMultiMap::~DiskMultiMap()
{

}

bool DiskMultiMap::createNew(const std::string& filename, unsigned int numBuckets)
{
	m_hash.createNew(filename);

	writeHeader(m_hash);

}

bool DiskMultiMap::openExisting(const std::string& filename)
{

}

void DiskMultiMap::close()
{
	if (!m_hash.isOpen)
		return;
	m_hash.close();
	return;
}

bool DiskMultiMap::insert(const std::string& key, const std::string& value, const std::string& context)
{

}

DiskMultiMap::Iterator DiskMultiMap::search(const std::string& key)
{

}

int DiskMultiMap::erase(const std::string& key, const std::string& value, const std::string& context)
{

}
