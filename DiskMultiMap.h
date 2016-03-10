#ifndef DISKMULTIMAP_H_
#define DISKMULTIMAP_H_

#include <string>
#include "MultiMapTuple.h"
#include "BinaryFile.h"

// CONSTANTS

enum element { FIRST, SECOND, THIRD, END };

const BinaryFile::Offset INVALID_NODE_LOCATION = 0;

const int MAX_ELEMENT_SIZE = 120 + 1; //INCLUDING nullbyte
const int MAX_NODE_SIZE = 3 * MAX_ELEMENT_SIZE;

const std::string CREATOR_MARK = "Made by DiskMultiMap";
// So we know the location of data in the file follows DiskMultiMap's hash
// Would be better to have this information stored with the BinaryFile object itself,
// but since I'm not allowed to alter its declaration/implementation,
// need to go about things somewhat sloppily and complain about it in the comments.
// (What am I, a YouTuber?)


const BinaryFile::Offset HEADER_LENGTH = sizeof(long) + sizeof(CREATOR_MARK);
// If it's a file we made, sizeof(long) corresponds to m_numBuckets
// and sizeof(CREATOR_MARK) is the length of the 'watermark'

const int TEMP_SIZE = MAX_ELEMENT_SIZE;

const char VALUE_SEPARATOR = '\0';

const BinaryFile::Offset USED_FLAG_LENGTH = sizeof(char); //just a '0' or '1'
const char IN_USE = '1';
const char NOT_IN_USE = '0';

const BinaryFile::Offset NEXT_NODE_LENGTH = sizeof(long);
const BinaryFile::Offset PRE_TUPLE_LENGTH = USED_FLAG_LENGTH + NEXT_NODE_LENGTH;

const BinaryFile::Offset EXTRA_SIZE = PRE_TUPLE_LENGTH + 1;
// +1 for nullbyte at very end of Node

const BinaryFile::Offset NODE_FILE_SIZE = MAX_NODE_SIZE + EXTRA_SIZE;



class DiskMultiMap
{
public:

	class Iterator
	{
	public:
		Iterator();
		DiskMultiMap::Iterator::Iterator(const std::string& key, DiskMultiMap* map);
		DiskMultiMap::Iterator::Iterator(const BinaryFile::Offset& bIndex, DiskMultiMap* map);
			// You may add additional constructors
		bool isValid() const;
		Iterator& operator++();
		MultiMapTuple operator*();

	private:
		DiskMultiMap* m_map;
		bool m_valid;
		BinaryFile::Offset m_bIndex;
		//int m_index;
		//std::string m_key;
		//std::string m_value;
		//std::string m_context;
		void setValid(bool x) { m_valid = x; }
		bool checkValidity();
		
		//BinaryFile::Offset DiskMultiMap::Iterator::giveCurrentNodeByteIndex();

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
	int hash(const std::string& input);
	BinaryFile::Offset giveHeadByteIndex(const int& index) const;
	bool writeHeader();
	bool DiskMultiMap::readHeader();
	char* DiskMultiMap::copyNode(BinaryFile::Offset source, BinaryFile::Offset destination);



	// functions dealing directly with Nodes' information
	// Mostly used by DiskMultiMap::Iterator
	// We assume that the passed-in Offset points to the beginning of a Node struct

	BinaryFile::Offset DiskMultiMap::giveNextNodeByte(BinaryFile::Offset bIndex);
	BinaryFile::Offset DiskMultiMap::giveNextNodeLocation(BinaryFile::Offset bIndex);
	BinaryFile::Offset DiskMultiMap::giveUsedByteIndex(BinaryFile::Offset bIndex) const;
	bool DiskMultiMap::isNodeUsed(BinaryFile::Offset bIndex);
	bool DiskMultiMap::isTerminalNode(BinaryFile::Offset bIndex);

	BinaryFile::Offset DiskMultiMap::giveTupleElementByte(element e, BinaryFile::Offset bIndex);
	std::string DiskMultiMap::giveTupleElement(element e, BinaryFile::Offset bIndex);

	void DiskMultiMap::setNextTo(BinaryFile::Offset next, BinaryFile::Offset current);
	void DiskMultiMap::setUsedFlag(bool x, BinaryFile::Offset bIndex);

	void DiskMultiMap::writeTupleInNode(BinaryFile::Offset loc, const std::string& key, const std::string& value, const std::string& context);


	// Your private member declarations will go here
};

#endif // DISKMULTIMAP_H_
