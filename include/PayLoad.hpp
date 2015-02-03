#ifndef PAY_LOAD_HPP
#define PAY_LOAD_HPP

#include "SpatialObject.hpp"
#include "BufferedFile.hpp"
#include <vector>
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

		PayLoad();

		~PayLoad();

		void create(string indexFileStem,uint32 pageSize,uint64 objectsPerPage,uint32 objectSize,SpatialObjectType objectType);

		void load(string indexFileStem);

		bool putPage(vector<SpatialObject*>& itemArray);

 		bool getPage(vector<SpatialObject*>& itemArray,int pageId);
	};
}

#endif
