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

const char VALUE_SEPARATOR = '\0';

const BinaryFile::Offset NEXT_NODE_LENGTH = sizeof(long);

const BinaryFile::Offset NODE_FILE_SIZE = NEXT_NODE_LENGTH + MAX_NODE_SIZE + 2;
	// +2 for VALUE_SEPARATOR and for nullbyte at very end of Node


///////////////////////////////
//// Iterator functions ///////
///////////////////////////////

DiskMultiMap::Iterator::Iterator()
{
	m_valid = false;
}

DiskMultiMap::Iterator::Iterator(const std::string& key, DiskMultiMap* map)
{
	m_map = map;
	m_key = key;
	m_index = m_map->hash(key);
	
	//determine validity

	if (m_map->m_creatorMark != CREATOR_MARK)
		m_valid = false;
	else
	{
		checkValidity();
	}
}

bool DiskMultiMap::Iterator::isValid() const
{
	return m_valid;
}

bool DiskMultiMap::Iterator::checkValidity()
{
	char ch;
	if (!m_map->m_hash.read(ch, m_map->giveUsedByteIndex(m_index)))
	{
		m_valid = false;
		return false; //couldn't read file!
	}
	else if ((ch - '0') == 0)
		m_valid = false;
	else
		m_valid = true;

	return true; //read file OK
}

DiskMultiMap::Iterator& DiskMultiMap::Iterator::operator++()
{
	if (!isValid())
		return *this;
	//otherwise increment up the MultiMap
	char temp[sizeof(long) + 1];

	m_map->m_hash.read(temp, sizeof(long), m_map->giveNodeByteIndex(m_index));
	temp[sizeof(long)] = '\0';

	long nextNode = atol(temp);
	if (nextNode == 0) //++'d on a terminal Node
	{
		setValid(false);
	}
	else
	{
		m_index = nextNode;
	}

	return *this;
}

MultiMapTuple DiskMultiMap::Iterator::operator*()
{
	MultiMapTuple m;
	m.context = "";
	m.key = "";
	m.value = "";

	if (!isValid())
		return m;
	else
	{
		m.key = m_key;

		char temp[MAX_NODE_SIZE + 2];

		//first is value
		m_map->m_hash.read(temp, MAX_NODE_SIZE + 2, //has nullbyte at end
			m_map->giveNodeByteIndex(m_index) + NEXT_NODE_LENGTH);

		m.value.assign(temp);

		//second is context
		// we should start reading m.value.size() + sizeof(value_separator) to the right
		//  and read that many fewer bytes

		m_map->m_hash.read(temp, MAX_NODE_SIZE + 2 - (m.value.size() + 1), //has nullbyte at end
			m_map->giveNodeByteIndex(m_index) + NEXT_NODE_LENGTH + (m.value.size() + 1));

		m.context.assign(temp);


		//done!

		return m;

	}
}


///////////////////////////////////
//// DiskMultiMap functions ///////
///////////////////////////////////



DiskMultiMap::DiskMultiMap()
{
	m_numBuckets = 0;
	m_headerLength = 0;
	m_creatorMark = "";
	//m_used?
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
//  2) numBuckets nullbytes, to function as my vector of bools to determine
//			whether a bucket is in use
//	3) (numBuckets * NODE_FILE_SIZE) nullbytes
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


	//init'ing usedVector
	for (int i = 0; i < numBuckets; i++)
		if (!m_hash.write('\0', m_hash.fileLength()))
			return false;

	// writing filler...
	for (int i = 0; i < numBuckets; i++)
	{
		

		for (int j = 0; j < (NODE_FILE_SIZE); j++)
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
//	2. a long that holds the index of the next Node in the list (0<=x<m_numBuckets),
//			or the value 0 if the Node is a terminal Node.
//	   When inserting a new Node and the hash-determined index is already filled,
//			the program will check the proceeding index until it finds an unused slot
// (sizeof(2) == sizeof(long))
//	3. the 'value' and 'context' data in C-string form.
//			In between the two values will be a VALUE_SEPARATOR.
//			At the [MAX_NODE_SIZE]'th index will be a nullbyte.
// (sizeof(3) == MAX_NDOE_SIZE + 1)
bool DiskMultiMap::insert(const std::string& key, const std::string& value, const std::string& context)
{
	if (!m_hash.isOpen())
		return false;

	int hashed = DiskMultiMap::hash(key);

	if (hashed >= m_numBuckets) //then I screwed up in DiskMultiMap::hash() somehow
		return false;

	DiskMultiMap::Iterator it(key, this);

	BinaryFile::Offset nb = giveNodeByteIndex(hashed);
	BinaryFile::Offset fb = giveUsedByteIndex(hashed);

	char ch;

	if (!m_hash.read(ch, fb))
		return false;
	if ((ch - '0') == 0) //is it not in use?
	{

	}



}

DiskMultiMap::Iterator DiskMultiMap::search(const std::string& key)
{

}

int DiskMultiMap::erase(const std::string& key, const std::string& value, const std::string& context)
{

}




// private member functions

int DiskMultiMap::hash(const std::string& input)
{
	std::hash<std::string> str_hash;
	int hashValue = str_hash(input); //use STL's hash
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

BinaryFile::Offset DiskMultiMap::giveNodeByteIndex(const int& index) const
{
	return ((HEADER_LENGTH + m_numBuckets) //header + usedVector
		+ ( (NODE_FILE_SIZE) * index) //offset by index*nodesize
		);
}

BinaryFile::Offset DiskMultiMap::giveUsedByteIndex(const int& index) const
{
	return HEADER_LENGTH + index;
}

