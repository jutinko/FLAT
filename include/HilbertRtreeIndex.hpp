#ifndef RTREE_INDEX_HPP
#define RTREE_INDEX_HPP

#include "DataFileReader.hpp"
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
	class HilbertRtreeIndex
	{
	public:
		SpatialIndex::IStorageManager* rtreeStorageManager;
		SpatialIndex::ISpatialIndex * tree;
		SpatialObjectType objectType;

		HilbertRtreeIndex();

		~HilbertRtreeIndex();

		void buildIndex(SpatialObjectStream* input,string indexFileStem);

		void loadIndex(string indexFileStem);
	};
}
#endif
