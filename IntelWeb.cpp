#include "IntelWeb.h"

const double BUCKET_FACTOR = 2;
const std::string POSTSTRING_MACHINES = "_machines_hash_table.dat";
const std::string POSTSTRING_WEBSITES = "_websites_hash_table.dat";
const std::string POSTSTRING_DOWNLOADS = "_downloads_hash_table.dat";
const std::string POSTSTRING_PREVALENCES = "_prevalences_hash_table.dat";

std::vector<std::string> poststrings =
{ POSTSTRING_MACHINES , POSTSTRING_WEBSITES, POSTSTRING_DOWNLOADS, POSTSTRING_PREVALENCES };


IntelWeb::IntelWeb()
{
	//set up our vector
	tables.push_back(machines);
	tables.push_back(websites);
	tables.push_back(downloads);
	
	p_buckets = 0;

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
	return false;
}

void IntelWeb::close()
{
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
	p_buckets = 0;
	
	// first perform dry run
	char ch;

	for (int i = 0; i < input.fileLength(); i++)
	{
		input.read(ch, i);
		if (iswspace(i)) 
			p_buckets++;
		//whitespace delineates the elements of a future Tuple (' ')
		// and future Tuples from each other ('\n')
	}

	// now we have the maximum possible number of buckets 'prevalences' could need
	// set up prevalences

	if (!prevalences.isOpen())
	{
		exit(10); //should have been opened...
	}

	//set up prevalences
	// Will save as longs
	long zero = 0;

	for (int i = 0; i < p_buckets; i++)
	{
		prevalences.write(zero, prevalences.fileLength());
	}


	// now will go through and start inserting

	char temp[MAX_ELEMENT_SIZE];
	std::string s;
	std::string key, value, context;
	std::vector<std::string> elements = { key, value, context };
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
			elements[i] = s;
			s = "";
			j++;
			if (j == elements.size()) //completed full Tuple
			{
				for (int k = 0; k < elements.size(); k++)
				{
					
				}
			}
		}
	}



	return false;
}

unsigned int IntelWeb::crawl(const std::vector<std::string>& indicators, unsigned int minPrevalenceToBeGood, std::vector<std::string>& badEntitiesFound, std::vector<InteractionTuple>& interactions)
{
	return 0;
}

bool IntelWeb::purge(const std::string & entity)
{
	return false;
}




void IntelWeb::closeAll()
{
	for (int i = 0; i < tables.size(); i++)
		tables[i].close();

	if (prevalences.isOpen())
		prevalences.close();
}

bool IntelWeb::createNewAll(std::string filePrefix, int maxDataItems)
{
	bool result = true;

	for (int i = 0; i < tables.size(); i++)
	{
		if (!tables[i].createNew(filePrefix + poststrings[i], BUCKET_FACTOR * maxDataItems))
			result = false;
	}

	if (!prevalences.createNew(filePrefix + POSTSTRING_PREVALENCES))
		result = false;

	return result;
}

bool IntelWeb::openExistingAll(std::string filePrefix)
{
	bool result = true;

	for (int i = 0; i < tables.size(); i++)
	{
		if (!tables[i].openExisting(filePrefix + poststrings[i]))
			result = false;
	}
	if (!prevalences.openExisting(filePrefix + POSTSTRING_PREVALENCES))
		result = false;

	if (result == true)
	{
		//can get p_buckets from prevalences's length
		p_buckets = prevalences.fileLength() / sizeof(long);
	}


	return result;
}



// Prevalence shenanigans

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
	long hashed = hash(input, p_buckets);

	return (hashed * sizeof(long));
}

void IntelWeb::setPIndex(BinaryFile::Offset value, std::string & input)
{
	BinaryFile::Offset bIndex = givePIndex(input);

	prevalences.write(value, bIndex);

	return;
}


