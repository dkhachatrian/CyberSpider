#ifndef INTELWEB_H_
#define INTELWEB_H_

#include "InteractionTuple.h"
#include "DiskMultiMap.h"
#include <string>
#include <vector>
#include <queue>

const double BUCKET_FACTOR = 2;
const std::string POSTSTRING_MACHINES = "_machines_hash_table.dat";
const std::string POSTSTRING_WEBSITES = "_websites_hash_table.dat";
const std::string POSTSTRING_DOWNLOADS = "_downloads_hash_table.dat";
const std::string POSTSTRING_ASSOCIATIONS = "_associations_hash_table.dat";

const char IS_MALICIOUS = '1';
const char IS_NOT_MALICIOUS = '0';

enum KeyType { machine, website, download };

const std::string POSTSTRING_PREVALENCES = "_prevalences_hash_table.dat";

std::vector<std::string> poststrings =
{ POSTSTRING_MACHINES , POSTSTRING_WEBSITES, POSTSTRING_DOWNLOADS, POSTSTRING_ASSOCIATIONS };





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
			std::vector<InteractionTuple>& badInteractions
			);
        bool purge(const std::string& entity);

private:
	DiskMultiMap* machines;
	DiskMultiMap* websites;
	DiskMultiMap* downloads;
	DiskMultiMap* associations; //contains all with 'key' ANYWHERE IN TUPLE

	class SimpleHashTable :public BinaryFile
	{
	public:
		SimpleHashTable(int x)
		{
			m_sizeOfNode = x;
		}

		void getValue(const std::string & input, std::string& output)
		{
			char temp[100];
			BinaryFile::Offset bIndex = giveBIndex(input);

			read(temp, nodeSize(), bIndex);
			temp[nodeSize()] = '\0';

			output.assign(temp);
			return;
		}

		BinaryFile::Offset giveBIndex(const std::string & input)
		{
			long hashed = m_owner->hash(input, giveNumberOfBuckets());

			return (hashed * nodeSize());
		}

		void setValue(BinaryFile::Offset value, const std::string & input)
		{
			BinaryFile::Offset bIndex = giveBIndex(input);

			write(value, bIndex);

			return;
		}

		BinaryFile::Offset giveNumberOfBuckets()
		{
			return fileLength() / nodeSize();
		}

		BinaryFile::Offset nodeSize() const
		{
			return m_sizeOfNode;
		}

	private:
		Offset m_sizeOfNode;
		IntelWeb* m_owner;
	};

	void IntelWeb::changePrevalenceBy(long x, std::string& input);

	std::vector<SimpleHashTable*> simples;
	SimpleHashTable* prevalences; //will act differently from hash tables above
	SimpleHashTable* m_maliciousFlags;
	//SimpleHashTable* m_originalAssociations; //hash v1,v2,v3

	std::vector<DiskMultiMap*> tables;
	long m_buckets_iw;
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

	//long getPrevalenceNumber(const std::string& input);
	//BinaryFile::Offset givePIndex(const std::string& input);
	//void setPIndex(BinaryFile::Offset value, std::string& input);

	KeyType determineKeyType(std::string input);

	InteractionTuple IntelWeb::makeInteractionTuple(MultiMapTuple m);

	void makeAssociations(std::string v1, std::string v2, std::string v3);

	void IntelWeb::retrieveAssociations(std::string key, std::queue<MultiMapTuple> origins, std::queue<DiskMultiMap::Iterator> itrs);
	/*
	template <typename T>
	bool IntelWeb::isALessThanB(T a, T b, element e)
	{
		return (a[e] < b[e]);
	}
	*/



	/*
	struct numberedString
	{
		numberedString(std::string st, int num)
		{
			s = st;
			x = num;
		}

		//checking by string
		bool operator<(const numberedString& other) const
		{
			return (this->s < other.s);
		}

		std::string s;
		int x;
	};
	*/
	// Your private member declarations will go here
};


// helper functions (predicate functions)


bool isALessThanB_string(std::string a, std::string b);

bool isEqualToB(MultiMapTuple a, MultiMapTuple b) 
{
	return ((a.key == b.key) && (a.value == b.value) && (a.context == b.context));
}

bool isALessThanB(InteractionTuple a, InteractionTuple b);


#endif // INTELWEB_H_
                
