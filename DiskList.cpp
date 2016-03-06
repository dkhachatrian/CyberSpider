#include "DiskList.h"
#include <string>
//#include <cstring>

const int MAX_STR_LENGTH = 255;

DiskList::DiskList(const std::string& filename)
{
	m_file.createNew(filename);
	unusedBytes = 0;
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

	char temp[MAX_STR_LENGTH + 1];


	//copy over everything toward the front to make space for new entry
	//will do the copying in chunks, first of MAX_STR_LENGTH + 1, then of what's left

	if ((strlen(data) + 1) > unusedBytes) //then we have to shove things over to make space for it
	{
		int i = m_file.fileLength(); //original file length
		int shift = strlen(data) + 1;

		while (i > (MAX_STR_LENGTH + 1))
		{
			if (!m_file.read(temp, MAX_STR_LENGTH + 1, i - (MAX_STR_LENGTH + 1) - 1))
				return false; //get last chunk of data

			if (!m_file.write(temp, MAX_STR_LENGTH + 1, i - (MAX_STR_LENGTH + 1) - 1 + shift))
				return false;
			//write it strlen(data) over to the right to make space for data at the front

			i -= (MAX_STR_LENGTH + 1);
			//move back another temp string length	
		}

		//now i <= MAX_STR_LENGTH + 1

		if (!m_file.read(temp, i, 0))
			return false; //get remaining chunk of data at front of file
		if (!m_file.write(temp, i, shift))
			return false; //shift it over



									  //now we can finally insert our new data at the front!
		if (!m_file.write(data, strlen(data) + 1, 0))
			return false; //adds str + vanguard

												 //and we're done!

												 //printAll();

		return true;
	}


	//otherwise, we have enough leftover space to make space at the front

	//so we'll shift things to the back as we encounter unused space

	int i = m_file.fileLength() - 1; //start at last char of file
	int j = 0;
	char ch = ' ';
	int totalShifted = 0;
	//int targetZeros = strlen(data) + 1;
	int numZeros = 0;
	int shift = strlen(data) + 1; //str + nullbyte


								  // If we have previously push_front'd a Node that could fit in (unusedBytes_old) bytes,
								  // we will have put all of the nullbytes at the front of our document.
								  // Because of this upkeep requirement,
								  // and because (I'm assuming that) nullbytes can't be contained in a node
								  // this means we can skip the while loop if the first (unusedBytes) of the file are nullbytes

								  // so let's see if we can skip the while loop!

	if (unusedBytes > 0 && m_file.read(temp, unusedBytes, 0))
	{
		bool canSkipLoop = true;
		
		for (int k = 0; k < unusedBytes; k++)
		{
			if (temp[k] != '\0')
			{
				canSkipLoop = false;
				break;
			}
			//otherwise, we've found an unused byte, so we can add it to our totalShifted
			totalShifted++;
		}

		if (canSkipLoop)
		{
			if (!m_file.write(data, strlen(data) + 1, unusedBytes - (strlen(data) + 1)))
				return false;
			//shifted just enough to copy over data + nullbyte
			//let's update unusedBytes
			unusedBytes -= (strlen(data) + 1);

			//and let's put our nullbytes at the front (unusedBytes) bytes of data
			// (since we've 'defragmented' the rest of the file)

			for (int k = 0; k < unusedBytes; k++)
				m_file.write('\0', k);

			// because of this upkeep requirement,
			// and because (I'm assuming that) nullbytes can't be contained in a node
			// this means we can skip the while loop if the first (unusedBytes) of the file are nullbytes


			return true;
		}

		//otherwise, we get out of if condition body and enter while loop
	}


	while ((i - numZeros - totalShifted) >= 0)
	{
		if (!m_file.read(ch, i - numZeros)) //should never be the case
			return false;

		if (ch == '\0')
		{
			numZeros++;
			continue;
		}

		else if (ch != '\0')
		{
			if (numZeros == 0) //mid-Node
			{
				i--; //keep shifting
				continue;
			}

			if (numZeros == 1) //just saw vanguard of Node. No shifting
			{
				//essentially, I "shift" all the bytes on the left to the right by (numZeros - 1), i.e., zero
				i -= numZeros;
				numZeros = 0;
				continue;
			}
			else if (numZeros > 1)
			{
				//shift everything to the right by (numZeros - 1)
				shift = numZeros - 1;
				j = i;
				//copypasta from unusedBytes condition body, with j's instead of i's

				while (j > (MAX_STR_LENGTH + 1))
				{
					if (!m_file.read(temp, MAX_STR_LENGTH + 1, j - (MAX_STR_LENGTH + 1) - 1))
						return false; //get last chunk of data

					if (!m_file.write(temp, MAX_STR_LENGTH + 1, j - (MAX_STR_LENGTH + 1) - 1 + shift))
						return false;
					//write it strlen(data) over to the right to make space for data at the front

					j -= (MAX_STR_LENGTH + 1);
					//move back another temp string length	
				}

				//now j <= MAX_STR_LENGTH + 1

				if (!m_file.read(temp, j - totalShifted - shift, totalShifted))
					return false; //get remaining chunk of data at front of file (except junk already shifted at front)
				if (strlen(temp) != 0) //if I'm not just shifting over nullbytes
					if (!m_file.write(temp, j - totalShifted - shift, totalShifted + shift))
						return false; //shift the remaining chunk of data over

																						// should have shifted everything to the right correctly

				totalShifted += shift; // keep track of how many unused bytes we've shifted
									   // (so we don't recopy the front of the file, to be overwritten, repeatedly)

									   // we have moved the front of the file over to where i is at the moment
									   // so (i-numZeros) hasn't checked the bytes just to the left of it
									   // so DON'T change i

				numZeros = 0; //reset numZeros counter
				continue; //and keep going


			}
		}
	}

	// by now, shoved over things. First (unusedBytes) bytes of data is junk and unnecessary
	//		(as we've already copied it over to the right)
	// let's overwrite some of the junk with our useful data

	if (!m_file.write(data, strlen(data) + 1, unusedBytes - (strlen(data) + 1)))
		exit(1);
	//shifted just enough to copy over data + nullbyte
	//let's update unusedBytes
	unusedBytes -= (strlen(data) + 1);

	//and let's put our nullbytes at the front (unusedBytes) bytes of data
	// (since we've 'defragmented' the rest of the file)

	for (int k = 0; k < unusedBytes; k++)
		m_file.write('\0', k);

	// because of this upkeep requirement,
	// and because (I'm assuming that) nullbytes can't be contained in a node
	// this means we can skip the while loop if the first (unusedBytes) of the file are nullbytes


	return true;



}


bool DiskList::remove(const char* data)
{
	char temp[MAX_STR_LENGTH + 1];

	bool flag = false;

	for (int i = 0; i < m_file.fileLength() - strlen(data); i++) //while there's enough space for data to conceivably be in the file
	{
		if (!m_file.read(temp, strlen(data), i)) //copy over the next strlen(data) bytes of the file to temp
			break;
		temp[strlen(data)] = '\0'; //set nullbyte 

		if (strcmp(data, temp) == 0) //if they match
		{
			flag = true;
			//"delete" the Node
			//will signify deletion by replacing all chars in the length with '\0'
			for (int j = 0; j < strlen(data); j++)
			{
				if (!m_file.write('\0', i + j)) //for every index between i and (i+j-1), replace with nullbyte
					break;
			}
			unusedBytes += (strlen(data) + 1); //chars and its corresponding nullbyte now available
		}
		// keep checking for entire file (want to remove *all* instances of input parameter)
	}


	return flag;
}


void DiskList::printAll()
{
	if (m_file.fileLength() == 0)
		return;

	int i = 0;
	char temp;
	char s[MAX_STR_LENGTH + 1];
	while (i < m_file.fileLength())
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