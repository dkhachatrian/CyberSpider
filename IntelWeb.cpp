#include "IntelWeb.h"
#include <algorithm> // gives stl::sort


IntelWeb::IntelWeb()
{
	//set up our vector
	tables.push_back(machines);
	tables.push_back(websites);
	tables.push_back(downloads);
	tables.push_back(associations);
	
	simples.push_back(prevalences);
	simples.push_back(m_maliciousFlags);

	m_buckets_iw = 0;

}

IntelWeb::~IntelWeb()
{
	closeAll();

}

bool IntelWeb::createNew(const std::string & filePrefix, unsigned int maxDataItems)
{
	closeAll();

	bool result = createNewAll(filePrefix, maxDataItems);

	if (result == false) //one of the files didn't create properly
	{
		closeAll();
	}

	return result;
}

bool IntelWeb::openExisting(const std::string & filePrefix)
{
	closeAll();

	bool result = openExistingAll(filePrefix);

	if (result == false) //one of the files didn't create properly
	{
		closeAll();
	}

	return result;
}

void IntelWeb::close()
{
	closeAll();
}

// NEED TO ASK WHAT CONDITIONS CALL FOR WHAT RETURN VALUES
bool IntelWeb::ingest(const std::string & telemetryFile)
{
	// whenever I ingest a line, I insert it into all three tables. So:
	// - if I hash a machine ID in the machines hash table, it will hold every tuple that
	//		has that machine ID in the tuple
	// - if I hash a website in the websites hash table, it will hold every tuple that
	//		has that website in the tuple
	// - if I hash a download in the downloads hash table, it will hold every tuple that
	//		has that download in the tuple
	//
	// Also, whenever I ingest a line, I hash EACH ELEMENT OF THE TUPLE,
	//		and increment its prevalence by 1
	//
	// This requires an initial 'dry pass' of the telemetry file (opened as a BinaryFile),
	//		so I know what I need to modulo my hash against

	
	//will assume files are open...

	BinaryFile input;
	input.openExisting(telemetryFile);
	m_buckets_iw = 0;
	
	// first perform dry run
	char ch;

	for (int i = 0; i < input.fileLength(); i++)
	{
		input.read(ch, i);
		if (iswspace(i)) 
			m_buckets_iw++;
		//whitespace delineates the elements of a future Tuple (' ')
		// and future Tuples from each other ('\n')
	}

	// now we have the maximum possible number of buckets 'prevalences' could need
	// set up prevalences

	if (!prevalences->isOpen())
	{
		exit(10); //should have been opened...
	}

	//set up prevalences
	// Will save as longs
	long zero = 0;

	for (int i = 0; i < m_buckets_iw; i++)
	{
		prevalences->write(zero, prevalences->fileLength());
		m_maliciousFlags->write(IS_NOT_MALICIOUS, m_maliciousFlags->fileLength());
	}


	// now will go through and start inserting

	char temp[MAX_ELEMENT_SIZE];
	std::string s;
	std::string key, value, context;
	std::vector<std::string> elements = { key, value, context };
	std::vector<KeyType> keys;
	int j = 0;

	for (int i = 0; i < input.fileLength(); i++)
	{
		input.read(ch, i);
		if (!iswspace(i))
			s += ch;
		//whitespace delineates the elements of a future Tuple (' ')
		// and future Tuples from each other ('\n')
		else
		{
			elements[j] = s;
			s = "";
			j++;
			if (j == elements.size()) //completed full Tuple
			{
				for (int k = 0; k < elements.size(); k++)
				{
					std::string t = elements[k];

					// update prevalences
					changePrevalenceBy(1, t);

					//setPIndex(givePIndex(t) + 1, t); //increment prevalence of each
					
					keys.push_back(determineKeyType(t));
				}

				//now I know which string is which keytype

				// shove into appropriate initiator hash table
				switch(keys[0])
				{
				case machine:
					machines->insert(elements[0], elements[1], elements[2]);
					break;
				case website:
					websites->insert(elements[0], elements[1], elements[2]);
					break;
				case download:
					downloads->insert(elements[0], elements[1], elements[2]);
					break;
				}

				// shove all combinations of the three keys into the associations hash table
				makeAssociations(elements[0], elements[1], elements[2]);

			}
		}
	}

	// Done! Now have three hashes with original lines,
	// a prevalences hash table (which also has prevalences for frequently used websites),
	// and an associations hash table that contain

	return true;

	//return false;
}

unsigned int IntelWeb::crawl(const std::vector<std::string>& indicators,
	unsigned int minPrevalenceToBeGood,
	std::vector<std::string>& badEntitiesFound,
	std::vector<InteractionTuple>& badInteractions
	)
{
	std::queue<MultiMapTuple> toBeChecked;
	std::queue<MultiMapTuple> origins;
	std::queue<DiskMultiMap::Iterator> itrs;

	// set up initial queue...
	for (int i = 0; i < indicators.size(); i++)
	{
		m_maliciousFlags->setValue(IS_MALICIOUS, indicators[i]);
		// flag all indicators as malicious in hash table
		
		retrieveAssociations(indicators[i], toBeChecked, itrs);
			// original Tuples are now in toBeChecked, with corresponding iterators
			
	}

	MultiMapTuple m;
	//char temp[TEMP_SIZE];
	std::string temp;
	std::string key;
	long pr; //prevalence
	char mal; //malicious flag

	while (!toBeChecked.empty())
	{
		m = toBeChecked.front(); // get next in queue
		toBeChecked.pop(); //remove from queue
		key = m.key;

		// check if we've already marked as malicious
		m_maliciousFlags->getValue(key, temp);
		if (temp.size() != 1)
			exit(30); //I messed up...
		mal = temp[0];

		if (mal == IS_MALICIOUS) 
		{
			continue;	//we don't want to 're-check' it
				// this should also prevent double-counting of identical lines
		}


		// get prevalence of current key
		prevalences->getValue(key, temp);
		pr = stol(temp);

		if (pr < minPrevalenceToBeGood)	//		if its prevalence in prevalence hash table < minPrevalenceGood and it isn't already marked as malicious
		{
			m_maliciousFlags->setValue(IS_MALICIOUS, key);//	mark entity as malicious in hash table
			badEntitiesFound.push_back(key); //	push entry into badEntitiesFound
			retrieveAssociations(key, toBeChecked, itrs); //  push association'd values into toBeChecked, and push the indicator into indicators
			badInteractions.push_back(makeInteractionTuple(m)); //	search in each hash table for each element of tuple (use iterator?) to find interactionLine, and push into BadInteractions
		}
	}

	// when it gets here, found all possible bad entities and interactions
	// and they are stored in
	// badEntitiesFound (string vector) and
	// badInteractions(InteractionTuple vector)

	// now sort them using stl::sort
	
	sort(badEntitiesFound.begin(), badEntitiesFound.end(), isALessThanB_string);
	// ugly function name, I know, but currently in "let's finish this thing already" mode
	// and apparently C++ couldn't figure out what I meant without changing name...

	//badInteractions is first sorted by context, then from, then to

	// so create an appropriate operator< for interactionTuples and use that with sort

	sort(badInteractions.begin(), badInteractions.end(), isALessThanB);

	// we return the number of malicious entities found, which == badEntitiesFound.size()

	return badEntitiesFound.size();


	//std::vector<numberedString> contexts, froms, tos;

	//for (int i = 0; i < badInteractions.size(); i++)
	//{
	//	contexts.push_back(numberedString(badInteractions[i].context, i));
	//	froms.push_back(numberedString(badInteractions[i].from, i));
	//	tos.push_back(numberedString(badInteractions[i].to, i));
	//}

	//std::vector<std::vector<numberedString>*> fields = { &contexts, &froms, &tos };
	//int sorted = 0;
	//std::vector<numberedString> f;

	//// sort through each field
	//for (int i = 0; i < fields.size(); i++)
	//{
	//	f = *(fields[i]);



	//}


	// flag all indicators as malicious in hash table
	// make an empty queue<string> toBeCehcked
	// make an empty queue<string> for indicators
	// search associations hash table using indicator[i], pushing into queues
	// while queue isn't empty:
	//		if its prevalence in prevalence hash table < minPrevalenceGood and it isn't already marked as malicious
	//			mark entity as malicious in hash table
	//			push entry into badEntitiesFound
	//			mark association'd values as malicious, push them into toBeChecked, and push the indicator into indicators
	//			search in each hash table for each element of tuple (use iterator?) to find interactionLine, and push into BadInteractions
	//	
	//	sort badEntitiesFound and BadInteractions (mergeSort? stl::sort?)
	// 
	//
	//
	// return badEntitiesFound.size();

	return 0;
}

// predicate functions

bool isALessThanB_string(std::string a, std::string b)
{
	return (a < b);
}

bool isALessThanB(InteractionTuple a, InteractionTuple b)
{
	if (a.context < b.context)
		return true;
	else if (a.context > b.context)
		return false;

	else //a.context == b.context
	{
		//check froms
		if (a.from < b.from)
			return true;
		else if (a.from > b.from)
			return false;
		else //a.from == b.from
		{
			//check tos
			if (a.to < b.to)
				return true;
			else if (a.to > b.to)
				return false;
			else //a.to == b.to
			{
				//this means I'm comparing duplicates
				//but I'm not supposed to have any duplicates in the vector I pass in!
				//bad
				exit(39);
			}
		}
	}
}


bool IntelWeb::purge(const std::string & entity)
{
	// put all associations with entity into a queue
	// while queue isn't empty:
	//			search in each hash table for each element of tuple to find interactionLine, and push into BadInteractions
	//			set them

	return false;
}

InteractionTuple IntelWeb::makeInteractionTuple(MultiMapTuple m)
{
	InteractionTuple i(m.key, m.value, m.context);
	return i;
}

// assoc will contain all associations related to key
// origins will have the original Tuples the association was made from
// itrs will hold Iterators starting at the beginning of the List of the hash table containing the original Tuple
void IntelWeb::retrieveAssociations(std::string key, std::queue<MultiMapTuple> origins, std::queue<DiskMultiMap::Iterator> itrs)
{
	DiskMultiMap::Iterator ita = associations->search(key);
	std::string a, b, c;
	while (ita.isValid())
	{
		MultiMapTuple m = *ita;
		//assoc.push(m);
		// push association

		// figure out what hash table has the original line that caused the associations

		// in the associations table, a and b don't have meaning
		// so they can appear in either order in the other hash tables
		a = m.key;
		b = m.value;
		c = m.context;

		//depending on KeyType of key (==a), will search different hash table in different order

		KeyType t = determineKeyType(a);
		DiskMultiMap::Iterator itTrue;
		std::string s1, s2, s3;

		// order they were added in associations table was one of the following:
		// - key value context
		// - value key context
		// - context key value
		//
		// In all hash tables, Tuple is saved as (key, value, context)

		// will figure out what Map to iterate over, and order to check Tuple
		switch (t)
		{
		case machine:
			itTrue = machines->search(a);
			s1 = a;
			s2 = b;
			s3 = c;
			break;
		case website:
			itTrue = websites->search(a);
			s1 = b;
			s2 = a;
			s3 = c;
			break;
		case download:
			itTrue = downloads->search(a);
			s1 = c;
			s2 = a;
			s3 = b;
			break;
		}

		// when I originally inserted, I also put the original line into hash table
		// so don't need to iterate to look for something that's definitely there

		MultiMapTuple ori;
		ori.key = s1;
		ori.value = s2;
		ori.context = s3;
		origins.push(ori);
		itrs.push(itTrue);

		++ita; //go to next Node
	}
}



void IntelWeb::closeAll()
{
	for (int i = 0; i < tables.size(); i++)
		tables[i]->close();

	if (prevalences->isOpen())
		prevalences->close();
}

bool IntelWeb::createNewAll(std::string filePrefix, int maxDataItems)
{
	bool result = true;
	int numBuckets = maxDataItems;

	for (int i = 0; i < tables.size(); i++)
	{
		if (tables[i] == associations)
			numBuckets = 3 * maxDataItems;

		if (!tables[i]->createNew(filePrefix + poststrings[i], BUCKET_FACTOR * maxDataItems))
			result = false;
	}

	if (!prevalences->createNew(filePrefix + POSTSTRING_PREVALENCES))
		result = false;

	return result;
}

bool IntelWeb::openExistingAll(std::string filePrefix)
{
	bool result = true;

	for (int i = 0; i < tables.size(); i++)
	{
		if (!tables[i]->openExisting(filePrefix + poststrings[i]))
			result = false;
	}
	if (!prevalences->openExisting(filePrefix + POSTSTRING_PREVALENCES))
		result = false;

	if (result == true)
	{
		//can get m_buckets_iw from prevalences's length
		m_buckets_iw = prevalences->fileLength() / sizeof(long);
	}


	return result;
}



// Prevalence shenanigans
/*
long IntelWeb::getPrevalenceNumber(const std::string & input)
{
	char temp[sizeof(long) + 1];
	BinaryFile::Offset bIndex = givePIndex(input);

	prevalences.read(temp, sizeof(long), bIndex);
	temp[sizeof(long)] = '\0';

	return atol(temp);
}

BinaryFile::Offset IntelWeb::givePIndex(const std::string & input)
{
	long hashed = hash(input, m_buckets_iw);

	return (hashed * sizeof(long));
}

void IntelWeb::setPIndex(BinaryFile::Offset value, std::string & input)
{
	BinaryFile::Offset bIndex = givePIndex(input);

	prevalences.write(value, bIndex);

	return;
}
*/

// making huge assumptions about telemetry data
// (but such log data should be standardized, right?)
KeyType IntelWeb::determineKeyType(std::string input)
{
	std::string site_disc = "http://";
	if (input.size() > site_disc.size() && input.substr(0, site_disc.size()) == site_disc)
		return website;

	int i = 0;
	for (i = 0; i < input.size(); i++)
	{
		if (i == 0)
		{
			if (input[i] != 'm' && input[i] != 'M')
				break;
			else continue;
		}
		if (!isdigit(input[i]))
			break;
	}

	if (i == input.size()) //made it through loop
		return machine;
	else return download;

}

// order doesn't matter; when looking through associations, will check all Nodes
void IntelWeb::makeAssociations(std::string v1, std::string v2, std::string v3)
{
	associations->insert(v1, v2, v3);
	associations->insert(v2, v1, v3);
	associations->insert(v3, v1, v2);		
}


void IntelWeb::changePrevalenceBy(long x, std::string& input)
{
	std::string v;
	prevalences->getValue(input, v);
	long val = stol(v);
	prevalences->setValue(val + x, input);
}
