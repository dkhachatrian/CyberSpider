#include "DiskMultiMap.h"
#include "BinaryFile.h"
#include <functional>
#include <vector>
#include <queue>

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
	//m_key = key;
	m_bIndex = m_map->giveHeadByteIndex(m_map->hash(key));
		//for beginning index, will be where hash function puts us
	//m_index = m_map->hash(key);
	
	//determine validity

	//if (m_map->m_creatorMark != CREATOR_MARK)
	//	m_valid = false;
	//else
	{
		checkValidity();
	}
}

DiskMultiMap::Iterator::Iterator(const BinaryFile::Offset& bIndex, DiskMultiMap* map)
{
	m_map = map;
	m_bIndex = bIndex;

	//determine validity
	checkValidity();

}

bool DiskMultiMap::Iterator::isValid() const
{
	return m_valid;
}

bool DiskMultiMap::Iterator::checkValidity()
{
	if (m_bIndex == INVALID_NODE_LOCATION) //was sent to front of file
	{
		setValid(false); //that means I ++'d on a terminal Node
	}
	else
	{
		while (m_bIndex != 0)
		{
			if (!(m_map->isNodeUsed(m_bIndex))) //if I was plopped into a deleted Node
				m_bIndex = m_map->giveNextNodeLocation(m_bIndex); // skip past deleted Nodes
			else break; // and stop when you hit a valid Node
		}
		if (m_bIndex == 0 || !(m_map->isNodeUsed(m_bIndex)))
			setValid(false);
				//end of the line... (deleted Nodes are after all valid Nodes)
		else //found a still-in-use Node
			setValid(true);
	}

	return true; //return value doesn't matter...

}

DiskMultiMap::Iterator& DiskMultiMap::Iterator::operator++()
{
	if (!isValid())
		return *this;
	//otherwise increment up the MultiMap

	m_bIndex = m_map->giveNextNodeLocation(m_bIndex);
		//increment once

	while (m_bIndex != 0)
	{
		if(!(m_map->isNodeUsed(m_bIndex))) //if I'm at a deleted Node
			m_bIndex = m_map->giveNextNodeLocation(m_bIndex); // skip past deleted Nodes
		else break; // and stop when you hit a valid Node

		// (as programmed, should always end as invalid if the if condition was ever true)
	}
			// will stop when it hits beginning of file or valid Node


	checkValidity(); //will become invalid if it went to beginning of file

	return *this;
}

MultiMapTuple DiskMultiMap::Iterator::operator*()
{
	MultiMapTuple m;
	m.key = "";
	m.value = "";
	m.context = "";

	if (!isValid())
		return m;
	else
	{
		m.key = m_map->giveTupleElement(FIRST, m_bIndex);
		m.value = m_map->giveTupleElement(SECOND, m_bIndex);
		m.context = m_map->giveTupleElement(THIRD, m_bIndex);

		return m;

		////m.key = m_key;

		//char temp[MAX_NODE_SIZE + EXTRA_SIZE];
		//int delta = 0;

		//// first is key
		//m_map->m_hash.read(temp, MAX_NODE_SIZE + EXTRA_SIZE - delta, //has nullbyte at end
		//	m_map->giveHeadByteIndex(m_index) + NEXT_NODE_LENGTH + delta);

		//m.key.assign(temp);

		//delta += (m_key.size() + 1);


		////second is value
		//m_map->m_hash.read(temp, MAX_NODE_SIZE + EXTRA_SIZE - delta, //has nullbyte at end
		//	m_map->giveHeadByteIndex(m_index) + NEXT_NODE_LENGTH + delta);

		//m.value.assign(temp);

		//delta += (m.key.size() + 1);


		////third is context
		//// we should start reading m.value.size() + sizeof(value_separator) to the right
		////  and read that many fewer bytes

		//m_map->m_hash.read(temp, MAX_NODE_SIZE + EXTRA_SIZE - delta, //has nullbyte at end
		//	m_map->giveHeadByteIndex(m_index) + NEXT_NODE_LENGTH + delta);

		//m.context.assign(temp);


		////done!

		//return m;

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


	////init'ing usedVector
	//for (int i = 0; i < numBuckets; i++)
	//	if (!m_hash.write('\0', m_hash.fileLength()))
	//		return false;

	// writing some stuff...
	for (int i = 0; i < numBuckets; i++)
	{
		//long p = ((i+1) % numBuckets);
		//	// index of the proceeding bucket,
		//	// which is where each original bucket will point
		//
		//if (!m_hash.write(p, m_hash.fileLength()))
		//	return false;
		//	//writing pointer

		for (int j = 0; j < NODE_FILE_SIZE; j++)
			if (!m_hash.write(NOT_IN_USE, m_hash.fileLength()))
				return false;
					//writing empty Nodes
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
//	3. the 'key', 'value', and 'context' data in C-string form.
//			In between the values will be a VALUE_SEPARATOR.
//			At the [MAX_NODE_SIZE]'th index will be a nullbyte.
// (sizeof(3) == MAX_NDOE_SIZE + EXTRA_SIZE)
bool DiskMultiMap::insert(const std::string& key, const std::string& value, const std::string& context)
{
	if (!m_hash.isOpen())
		return false;

	std::vector<std::string> temp = { key, value, context };

	// ensure size fits
	for (int i = 0; i < temp.size(); i++)
		if (temp[i].size() >(MAX_ELEMENT_SIZE - 1)) // -1 to remove nullbyte
			return false;

	int hashed = DiskMultiMap::hash(key);

	if (hashed >= m_numBuckets) //then I screwed up in DiskMultiMap::hash() somehow
		exit(40);

	//DiskMultiMap::Iterator it = search(key);

	BinaryFile::Offset current = giveHeadByteIndex(hashed);

	// if head is empty, our entire linked list is emipty
	if (!isNodeUsed(current))
	{
		writeTupleInNode(current, key, value, context);
		return true;
	}
	
	while (!isTerminalNode(current)) //while we're not at the terminal Node
	{
		current = giveNextNodeLocation(current);
	}

	// determine how it's a terminal Node

	BinaryFile::Offset next = giveNextNodeLocation(current);

	if (next != INVALID_NODE_LOCATION) //if we point to a deleted Node location
	{
		//overwrite old data in 'next'
		BinaryFile::Offset b = next;
		writeTupleInNode(b, key, value, context);

		return true;
	}

	//otherwise, add to end of file
	else
	{
		BinaryFile::Offset b = m_hash.fileLength();
		setNextTo(b, current); //have current point to new Node
		writeTupleInNode(b, key, value, context);

		return true;
	}

	//should never get here
}



DiskMultiMap::Iterator DiskMultiMap::search(const std::string& key)
{
	DiskMultiMap::Iterator it(key, this);
	return it;
}

// Whenever a Node is erased, all tuples proceeding it are copied into the preceeding Node
// and the final Node location is set to unused.
// (If we then insert() a new Tuple, it will use this unused Node)
int DiskMultiMap::erase(const std::string& key, const std::string& value, const std::string& context)
{
	int numErased = 0;

	int hashed = hash(key);
	MultiMapTuple m;

	BinaryFile::Offset head = giveHeadByteIndex(hashed);
	BinaryFile::Offset current = head;

	BinaryFile::Offset next = giveNextNodeLocation(current);

	BinaryFile::Offset firstValidLoc = INVALID_NODE_LOCATION;
	BinaryFile::Offset firstInvalidLoc = INVALID_NODE_LOCATION;

	std::queue<BinaryFile::Offset> erased;

	bool deletedHead = false;
	bool checkedHead = false;


	// the Nodes are going to point in the order:
	// valid Nodes ... -> deleted Nodes ... -> INVALID_NODE_LOCATION
	// We only want to delete valid Nodes,
	// but we want to go all the way until the last deleted Node
	// so that we can link any Nodes deleted from this function call
	// to the end of the chain of deleted Nodes
	while (giveNextNodeLocation(current) != INVALID_NODE_LOCATION)
	{
		// we need to think about what to do when the entry in the original bucket is erased
		// (as this is our starting point)
		// We want the Node in this first bucket to be the first valid Node
		// (will make searching quicker)
		
		// So let's check if we deleted it

		next = giveNextNodeLocation(current);

		if (isNodeUsed(current))
		{
			if (!checkedHead && current == head) //if head and haven't checked
			{
				next = current; //will check head
				checkedHead = true; //remember we checked so we actually continue
			}

			//check the NEXT Node's information
			m.key = giveTupleElement(FIRST, next);
			m.context = giveTupleElement(SECOND, next);
			m.value = giveTupleElement(THIRD, next);

			if (m.key == key && m.value == value && m.context == context)
			{
				if (next == head) //if head
				{
					deletedHead = true; // remember we deleted the head
				}

				//keep track of where this location is (the NEXT one)
				erased.push(next);
				//set its flag to unused
				setUsedFlag(false, next);
				// count up a deleted item
				numErased++;
				// make current Node's nextNodeLocation be
				// whatever the next Node's nextNodeLocation is
				setNextTo(giveNextNodeLocation(next), current);

				// we've changed what the current's next points to
				// so start the loop again WITHOUT changing the Offset of current
				continue;
			}
			else
			{
				if (firstValidLoc == INVALID_NODE_LOCATION) //if this is our first valid Node
				{
					firstValidLoc = next; //remember its location
				}

				current = giveNextNodeLocation(current); //keep on trucking
				continue;
			}
		}
		else if (firstInvalidLoc == INVALID_NODE_LOCATION)
		{
			firstInvalidLoc = current;
		}
	}

	if (numErased != erased.size()) //then I messed up
		exit(5);


	// Keeping track of newly erased Nodes

	// Need to take special care if we deleted the Node that's in the main hash table (head)

	if (deletedHead)
	{
		if (firstValidLoc != INVALID_NODE_LOCATION) //if we found a valid Node
		{
			// will copy over valid Node
			// and act as if we deleted the Node at its original location

			copyNode(firstValidLoc, head);
				//(don't need to copy deleted head information)

			//act as if it were deleted
			setUsedFlag(false, firstValidLoc);
			erased.push(firstValidLoc);

			//but DON'T update counter. Already counted head as erased
			// (Just now, we 'inserted' one and 'removed' one)
			
			if (erased.front() == head) //just making sure...
				erased.pop(); //took care of head 
			else
				exit(5); //I messed up...
		}

		//otherwise, we have an empty linked list
		// everything after the head either used to be valid and is now invalid
		// or was already invalid
		// so head would be our very first invalid Node
		// which is perfect, as if we insert a valid Node, we'd insert into the head
		// so we can proceed as usual
	}


	// Now we're at the end of the chain of deleted Nodes
	// We will now set each Node's nextLocation to whatever's at the front of the queue,
	// moving our Offset location along with it

	while (!erased.empty())
	{
		setNextTo(erased.front(), current); //set pointer
		current = erased.front(); //change location
		erased.pop(); //pop from top
	}

	// now current is the very last newly deleted Node.
	// So let's set this to what used to be the first invalid location.

	setNextTo(firstInvalidLoc, current);

	// and we're done!

	return numErased;

}




// private member functions

//will return the destination's information as a char*
char* DiskMultiMap::copyNode(BinaryFile::Offset source, BinaryFile::Offset destination)
{
	char temp[NODE_FILE_SIZE];

	char result[NODE_FILE_SIZE];

	m_hash.read(temp, NODE_FILE_SIZE, source);
	m_hash.read(result, NODE_FILE_SIZE, destination); //to be returned
	m_hash.write(temp, NODE_FILE_SIZE, destination); //overwrite

	return result;
}

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



BinaryFile::Offset DiskMultiMap::giveHeadByteIndex(const int& index) const
{
	return ((HEADER_LENGTH + m_numBuckets) //header + usedVector
		+ ( (NODE_FILE_SIZE) * index) //offset by index*nodesize
		);
}


// Functions that interact with Nodes

// Accessors

BinaryFile::Offset DiskMultiMap::giveUsedByteIndex(BinaryFile::Offset bIndex) const
{
	return bIndex;
}



BinaryFile::Offset DiskMultiMap::giveNextNodeByte(BinaryFile::Offset bIndex)
{
	return (bIndex += USED_FLAG_LENGTH);
}


// returns an Offset that is the first byte of the proceeding Node
// (Sends to 0 if it was a terminal Node)
BinaryFile::Offset DiskMultiMap::giveNextNodeLocation(BinaryFile::Offset bIndex)
{
	//BinaryFile::Offset b = bIndex;
	bIndex = giveNextNodeByte(bIndex); //now at byteIndex portion

	char temp[sizeof(long) + 1];

	m_hash.read(temp, sizeof(long), bIndex);
	temp[sizeof(long)] = '\0';

	return (atol(temp));

}

// Terminal if points to beginning of file OR to a deleted Node
bool DiskMultiMap::isTerminalNode(BinaryFile::Offset bIndex)
{
	bIndex = giveNextNodeLocation(bIndex);

	return (bIndex == INVALID_NODE_LOCATION || !isNodeUsed(bIndex));

}


// Assumption: bIndex is at start of valid Node
bool DiskMultiMap::isNodeUsed(BinaryFile::Offset bIndex)
{
	char ch;
	m_hash.read(ch, bIndex);
	return (ch != NOT_IN_USE);
}

void DiskMultiMap::setUsedFlag(bool x, BinaryFile::Offset bIndex)
{
	char ch;
	bIndex = giveUsedByteIndex(bIndex);

	if (x)
		ch = IN_USE;
	else
		ch = NOT_IN_USE;

	m_hash.write(ch, bIndex);

	return;

}


BinaryFile::Offset DiskMultiMap::giveTupleElementByte(element e, BinaryFile::Offset bIndex)
{
	int i = 0;
	char ch;

	bIndex += PRE_TUPLE_LENGTH;

	while (i < e)
	{
		bIndex++;
		m_hash.read(ch, bIndex);

		if (ch == VALUE_SEPARATOR)
			i++;
	}

	bIndex++; //move off VALUE_SEPARATOR, onto first byte of next element
	return bIndex;

}

std::string DiskMultiMap::giveTupleElement(element e, BinaryFile::Offset bIndex)
{
	bIndex = giveTupleElementByte(e, bIndex);

	char temp[TEMP_SIZE];

	BinaryFile::Offset bEnd = giveTupleElementByte(END, bIndex);
	BinaryFile::Offset delta = bEnd - bIndex; //length from element to end of entire Node
		//string can not be any longer than that

	m_hash.read(temp, bIndex, delta + 1);
		//there should be a nullbyte in there somewhere
	
	std::string s(temp);
	return s;


}

// for Node in current, sets it 'next' pointer to value in next
void DiskMultiMap::setNextTo(BinaryFile::Offset next, BinaryFile::Offset current)
{
	current = giveNextNodeByte(current);

	m_hash.write(next, current);

	return;
}


void DiskMultiMap::writeTupleInNode(BinaryFile::Offset loc, const std::string& key, const std::string& value, const std::string& context)
{
	std::vector<std::string> temp = { key, value, context };

	BinaryFile::Offset b = giveTupleElementByte(FIRST, loc); //puts me at the very beginning of element area
															 //now write

	for (int i = 0; i < temp.size(); i++)
	{
		m_hash.write(temp[i].c_str(), temp[i].length(), b);
		//writes all chars of element
		m_hash.write(VALUE_SEPARATOR, b + temp[i].length());
		//writes nullbyte to cap string
		b += MAX_ELEMENT_SIZE;
		//shift over to start of next element
	}

	m_hash.write('\0', b); //cap off Node entry with fourth and final nullbyte

						   //set used flag of location to true
	setUsedFlag(true, loc);

	//return

}
