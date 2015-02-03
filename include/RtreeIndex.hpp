#ifndef RTREE_INDEX_HPP
#define RTREE_INDEX_HPP

#include "DataFileReader.hpp"
#include "SpatialQuery.hpp"
#include <SpatialIndex.h>

namespace FLAT
{
#define SORTING_FOOTPRINT_MB 400

	class rtree_stream : public SpatialIndex::IDataStream
	{
	public:
		SpatialObjectStream* i;
		bool first;

		rtree_stream (SpatialObjectStream* input);

		~rtree_stream();

		bool hasNext();

		uint32_t size();

		void rewind();

		SpatialIndex::IData* getNext();
	};


	/*
	 * Class responsible for managing the Rtree Index structure,
	 * This is an STR Style RTREE indexing
	 */
	class RtreeIndex
	{
	public:
		SpatialIndex::IStorageManager* rtreeStorageManager;
		SpatialIndex::ISpatialIndex* tree;
		SpatialObjectType objectType;

		RtreeIndex();

		~RtreeIndex();

		void buildIndex(SpatialObjectStream* input);

		void loadIndex(string indexFileStem, SpatialObjectType type);

    // Range queries
		void query(SpatialQuery *q, vector<SpatialObject *> *result);

    // kNn queries
		void kNNQuery(SpatialQuery *q, vector<SpatialObject *> *result);

		void setObjectType(SpatialObjectType type);
	};
}
#endif
