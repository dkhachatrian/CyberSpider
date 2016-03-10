#ifndef INTELWEB_H_
#define INTELWEB_H_

#include "InteractionTuple.h"
#include "DiskMultiMap.h"
#include <string>
#include <vector>

//class DiskMultiMap;

class IntelWeb
{
public:
        IntelWeb();
        ~IntelWeb();
        bool createNew(const std::string& filePrefix, unsigned int maxDataItems);
        bool openExisting(const std::string& filePrefix);
        void close();
        bool ingest(const std::string& telemetryFile);
        unsigned int crawl(const std::vector<std::string>& indicators,
                	   unsigned int minPrevalenceToBeGood,
                	   std::vector<std::string>& badEntitiesFound,
                	   std::vector<InteractionTuple>& interactions
                	  );
        bool purge(const std::string& entity);

private:
	DiskMultiMap machines, websites, downloads;
	BinaryFile prevalences; //will act differently from hash tables above
	std::vector<DiskMultiMap> tables;
	long p_buckets;
	//int expectedNumber;

	void closeAll();
	bool createNewAll(std::string filePrefix, int maxDataItems);
	bool openExistingAll(std::string filePrefix);
		//name refers to what the key for the multimap is
	long hash(const std::string& input, int numBuckets)
	{
		std::hash<std::string> str_hash;
		long hashValue = str_hash(input); //use STL's hash
		return (hashValue % numBuckets); //make sure it fits in the current file's bucket size
	}

	long getPrevalenceNumber(const std::string& input);
	BinaryFile::Offset givePIndex(const std::string& input);
	void setPIndex(BinaryFile::Offset value, std::string& input);

	// Your private member declarations will go here
};

#endif // INTELWEB_H_
                
