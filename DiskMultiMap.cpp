#include "DiskMultiMap.h"
#include "BinaryFile.h"
#include <functional>

const std::string CREATOR_MARK = "Made by DiskMultiMap";
	// So we know the location of data in the file follows DiskMultiMap's hash
	// Would be better to have this information stored with the BinaryFile object itself,
	// but since I'm not allowed to alter its declaration/implementation,
	// need to go about things somewhat sloppily and complain about it in the comments.
	// (What am I, a YouTuber?)


const BinaryFile::Offset HEADER_LENGTH = sizeof(long) + sizeof(CREATOR_MARK);
	// If it's a file we made, sizeof(long) corresponds to m_numBuckets
	// and sizeof(CREATOR_MARK) is the length of the 'watermark'

const int TEMP_SIZE = MAX_NODE_SIZE + 1;

DiskMultiMap::Iterator::Iterator()
{
	m_valid = false;
}

DiskMultiMap::Iterator::Iterator(BinaryFile::Offset startLoc, BinaryFile& hash)
{
	
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
	m_numBuckets = 0;
	m_headerLength = 0;
	m_creatorMark = "";

	//BinaryFile m_hash; //is this even initializing anything? ...
}

DiskMultiMap::~DiskMultiMap()
{
	DiskMultiMap::close();
}



// My DiskMultiMap's Binary File will be structured in the following order:
//	1) A header containing, in order:
//		- a long, saying the number of buckets this file holds
//		- the creator mark C-string "Made by DiskMultiMap", so we know how the data is stored
//	2) (numBuckets * (MAX_NODE_SIZE + 1)) nullbytes
//
// When createNew()'ing, DiskMultiMap will
//    store the header information in its private member variables.
bool DiskMultiMap::createNew(const std::string& filename, unsigned int numBuckets)
{
	m_hash.createNew(filename);

	m_numBuckets = numBuckets;
	m_creatorMark = CREATOR_MARK;

	if (!writeHeader()) //problems writing in header?
		return false;

	m_headerLength = HEADER_LENGTH;

	// writing filler...
	for (int i = 0; i < numBuckets; i++)
	{
		for (int j = 0; j < (MAX_NODE_SIZE + 1); j++)
			if (!m_hash.write('\0', m_hash.fileLength()))
				return false;
	}

	//ready to rumble!
	return true;

}

bool DiskMultiMap::openExisting(const std::string& filename)
{
	//if (m_hash.isOpen())
	DiskMultiMap::close(); //close anything that might be open

	if (!m_hash.openExisting(filename))
		return false;

	//otherwise, can attempt to read header
	if (!readHeader())
		return false;

	// if creator mark doesn't match, complain

	if (m_creatorMark != CREATOR_MARK)
	{
		cerr << "Cannot interpret file structure!" << endl;
		exit(2);
	}

	// otherwise, we're good to go

	return true;

	
}

void DiskMultiMap::close()
{
	if (m_hash.isOpen())
		m_hash.close();
	return;
}



// Each Node in my open hash table will have a structure as follows:
//	1. a single byte that is 0 when the Node is not in use and 1 when it is in use
//	(sizeof(1) == 1)
//	2. a long that holds the index of the first byte to the next Node in the list,
//			or the value 0 if the Node is a terminal Node.
//	   When inserting a new Node and the hash-determined index is already filled,
//			the program will check the proceeding index until it finds an unused slot
// (sizeof(2) == sizeof(long))
//	3. the 'value' and 'context' data in C-string form.
//			In between the two values will be a space character (' ').
//			At the [MAX_NODE_SIZE]'th index will be a nullbyte.
// (sizeof(3) == MAX_NDOE_SIZE + 1)
bool DiskMultiMap::insert(const std::string& key, const std::string& value, const std::string& context)
{
	



}

DiskMultiMap::Iterator DiskMultiMap::search(const std::string& key)
{

}

int DiskMultiMap::erase(const std::string& key, const std::string& value, const std::string& context)
{

}




// private member functions

BinaryFile::Offset DiskMultiMap::hash(const std::string& input)
{
	std::hash<std::string> str_hash;
	BinaryFile::Offset hashValue = str_hash(input); //use STL's hash
	return (hashValue % m_numBuckets); //make sure it fits in the current file's bucket size
}

bool DiskMultiMap::writeHeader()
{
	if (!m_hash.isOpen())
		return false;

	if (!m_hash.write(m_numBuckets, 0))
		return false; //filesize is size of long
	if (!m_hash.write(CREATOR_MARK.c_str(), CREATOR_MARK.length(), sizeof(long)))
		return false; //we know we can use hash to look up values

	return true;

}

bool DiskMultiMap::readHeader()
{
	if (m_hash.fileLength() < HEADER_LENGTH)
		return false; //effectively empty file

	char temp[TEMP_SIZE];


	// read in number of buckets

	if (!m_hash.read(temp, sizeof(long), 0))
		return false;

	temp[sizeof(long)] = '\0'; //cap C-string

	if(temp != nullptr)
		m_numBuckets = atol(temp);

	// read in creator mark

	if (!m_hash.read(temp, sizeof(CREATOR_MARK), sizeof(long)))
		return false;

	temp[sizeof(CREATOR_MARK)] = '\0'; //cap C-string

	if (temp != nullptr)
		m_creatorMark.assign(temp);

	//done!
	return true;

}