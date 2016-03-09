#include "DiskList.h"
#include <string>
//#include <cstring>

const int MAX_STR_LENGTH = 255;
const int UNUSED_BUFFER = 255;

DiskList::DiskList(const std::string& filename)
{
	m_file.createNew(filename);

	//will write in the initial number of unused byte location offset
	m_file.write(0, 0); //initial unused byte offset
	m_file.write(0, sizeof(int)); //initial locations of items, in order

	//unusedBytes = 0;
}


bool DiskList::push_front(const char* data)
{

	if (strlen(data) > MAX_STR_LENGTH) //strlen does NOT include +1 for nullbyte
	{
		return false;
	}

	if (m_file.fileLength() == 0)
	{
		if (!m_file.write(data, strlen(data) + 1, 0))
			return false;
		return true;
	}

	int unusedByteOffset = getUnusedByteOffset();

	char n[sizeof(int) + 1];

	int spaceSize;
	int spaceLoc;

	int k = m_file.fileLength();

	
	// pairs of numbers were written in order (location, size)
	// so going from backwards, I first get size, then I get location
	while (k > unusedByteOffset)
	{
		k -= sizeof(int);
		// size
		if (!m_file.read(n, k, sizeof(int)))
			return false;
		n[sizeof(int)] = '\0'; //building the int
		spaceSize = atoi(n);

		k -= sizeof(int); //move to location

		if (spaceSize < strlen(data) + 1) //but if we can't fit it here
		{
			//skip location of this too-small gap
			continue;
		}

		//otherwise, we found a good enough gap!

		//location
		if (!m_file.read(n, k, sizeof(int)))
			return false;
		n[sizeof(int)] = '\0'; //building the int
		spaceLoc = atoi(n);

		if (!m_file.write(data, strlen(data) + 1, spaceLoc)) //write it in
			break;
		// note that that spot is no longer empty
		for (int i = 0; i < 2 * sizeof(int); i++)
			m_file.write('\0', k + i); //will do this by setting nullbytes

		//and we're good!
		return true;
	}




	//otherwise, we need to shift things over by extraBytesNeeded to make space in the front


	char temp[MAX_STR_LENGTH + 1];

	char ch;
	shift = extraBytesNeeded;

	for (int i = m_file.fileLength() - 1; i >= (sizeof(int) + unusedBytes); i--)
	{
		if (!m_file.read(ch, i))
			return false;
		if (!m_file.write(ch, i + shift))
			return false;
	}
	//now there's just enough space, after my initial int, to write in my int
	if (!m_file.write(data, strlen(data) + 1, sizeof(int)))
		return false;

	//update unusedBytes (which is now zero), and we're done
	return (m_file.write(0, 0));

}


bool DiskList::remove(const char* data)
{
	char temp[MAX_STR_LENGTH + 1];


	int unusedByteOffset = getUnusedByteOffset();

	bool flag = false;
	char ch = ' ';

	//m_file.read(ch, m_file.fileLength() - 1);

	int i = sizeof(int); //start after 'pointer' (at the beginning of file)
	int j;

	while (i < unusedByteOffset - (strlen(data) + 1)) //while there's enough space for data to conceivably be in the file
	{
		j = i;
		while (j < unusedByteOffset)
		{
			if (!m_file.read(ch, j))
				break;
			if (j == i && ch == '\0') //if I started at a nullbyte,
			{
				//must be in the middle of some unused space
				//keep going
				while (j < unusedByteOffset && ch == '\0')
				{
					m_file.read(ch, j);
					j++;
				}
			}
			else
			{
				temp[j - i] = ch;
				j++;
				if (ch == '\0') //end of Node
				{
					break;
				}
			}
		}

		if (j == unusedByteOffset && temp[j-i] != '\0') //we don't have a valid Node,
				//and we've reached the end of the "NodeSpace" in file
			break; //then can't be Node, and we should leave the loop


		if (strcmp(data, temp) == 0) //if temp and target match
		{
			flag = true;
			//"delete" the Node
			//will overwrite by setting to nullbyte (to make printAll() function easier/quicker)
			for (int j = 0; j < strlen(data); j++)
				if (!m_file.write('\0', i + j))
					return false;
			//strlen(data) + 1 is already nullbyte


			// keep track of where this empty spot is (somewhere at the end of file)

			char zeros[2 * sizeof(int)];
			bool emptyNoted = false;
			int location = i;
			int size = strlen(data) + 1;

			//first look to see if there are now-invalid spots at the end of file
			// whose bytes we can reuse to hold a new location and size
			for (int k = m_file.fileLength() - 2 * sizeof(int); k >= unusedByteOffset; k -= (2 * sizeof(int)))
			{
				int j;
				m_file.read(zeros, 2 * sizeof(int), k);
				for (j = 0; j < 2 * sizeof(int); j++)
					if (zeros[j] != '\0')
					{
						break;
					}
				if (j != 2 * sizeof(int)) //not all zeros
					continue; //so still in use

				//otherwise, is empty. Can be replaced
				m_file.write(location, k); //write location first
				m_file.write(size, k + sizeof(int)); //and size second
				emptyNoted = true; //set flag
				break;
			}

			if (emptyNoted)
				continue;
			else
			{
				if (!m_file.write(location, m_file.fileLength())) //location of unused space
					break;
				if (!m_file.write(size, m_file.fileLength())) //size of unused space
					break;
				continue;
			}

		}
		// check the next Node
		i = j;
	}

	//out of loop!

	return flag;
}


void DiskList::printAll()
{

	int unusedByteOffset = getUnusedByteOffset();


	int i = sizeof(int);
	char temp;
	while (i < unusedByteOffset)
	{
		m_file.read(temp, i); //take the next char in the file
		if (temp == '\0') //if nullbyte
		{
			i++; //advance
			continue; //skip printing any nullbytes that occur without a matching str
		}

		else
		{
			while (temp != '\0')
			{
				cout << temp; //send out char
				i++; //advance a char
				if (!m_file.read(temp, i)) //take char in i'th position of file
					break;
			}
			// reached '\0', so end of Node
			// (beginning of while loop will handle how to deal with '\0', so just print a newline)
			cout << endl;
		}
	}

	return;

}


//will give the byte in memory where the list of (# unused bytes, memory location) 'tuples' will be held
//this int value will be stored at the front of the file
inline int DiskList::getUnusedByteOffset()
{
	char s[sizeof(int) + 1];

	m_file.read(s, sizeof(int), 0);
	s[sizeof(int)] = '\0'; //put an end to CString
	return atoi(s); //convert to int, the number of unusedBytes

}

