#ifndef DISKMULTIMAP_H_
#define DISKMULTIMAP_H_

#include <string>
#include "MultiMapTuple.h"

const int MAX_NODE_SIZE = 120; //NOT including nullbyte

class DiskMultiMap
{
public:

	class Iterator
	{
	public:
		Iterator();
		Iterator(BinaryFile::Offset startLoc, BinaryFile& hash);
		// You may add additional constructors
		bool isValid() const;
		Iterator& operator++();
		MultiMapTuple operator*();

	private:
		//for 'storage' purposes
		bool m_valid;
		std::string m_key;
		std::string m_value;
		std::string m_context;
		// Your private member declarations will go here
	};

	DiskMultiMap();
	~DiskMultiMap();
	bool createNew(const std::string& filename, unsigned int numBuckets);
	bool openExisting(const std::string& filename);
	void close();
	bool insert(const std::string& key, const std::string& value, const std::string& context);
	Iterator search(const std::string& key);
	int erase(const std::string& key, const std::string& value, const std::string& context);

private:
	BinaryFile m_hash;
	BinaryFile::Offset m_numBuckets;
	BinaryFile::Offset m_headerLength;
	std::string m_creatorMark;
	BinaryFile::Offset hash(const std::string& input);
	bool writeHeader();
	bool DiskMultiMap::readHeader();

	// Your private member declarations will go here
};

#endif // DISKMULTIMAP_H_
