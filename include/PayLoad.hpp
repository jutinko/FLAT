#ifndef PAY_LOAD_HPP
#define PAY_LOAD_HPP

#include "SpatialObject.hpp"
#include "BufferedFile.hpp"
#include "Metadata.hpp"
#include <vector>
#include <unordered_map>
using namespace std;

namespace FLAT
{
	/*
	 * Class Responsible for Managing the Payload on our Index
	 */
	class PayLoad
	{
	public:
		BufferedFile* file;      //Pointer to file containing data pages for writing
		string filename;
		uint32 pageSize;
		uint64 objectsPerPage;
		uint32 objectSize;
		bool isCreated;
		SpatialObjectType objType;
    unordered_map<uint64, vector<SpatialObject*> > table;

		PayLoad();

		~PayLoad();

		void create(string indexFileStem,uint32 pageSize,uint64 objectsPerPage,uint32 objectSize,SpatialObjectType objectType);

    void createInMemory(SpatialObjectType objectType);

		void load(string indexFileStem);

		bool putPage(vector<SpatialObject*>& itemArray);

		bool putPageInMemory(uint64 id, vector<SpatialObject*>& itemArray);

 		bool getPage(vector<SpatialObject*>& itemArray,int pageId);

 		bool getPageInMemory(vector<SpatialObject*>& itemArray, uint64 pageId);
	};
}

#endif
