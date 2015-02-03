#include "PriorityRtreeIndex.hpp"

namespace FLAT
{
	rtree_stream::rtree_stream (SpatialObjectStream* input)
	{
		i = input;
		first = true;
	}

	rtree_stream::~rtree_stream()
	{
	}

	bool rtree_stream::hasNext()
	{
		if (first) {first=false;return true;}
		return i->hasNext();
	}

	uint32_t rtree_stream::size()
	{
		abort();
	}

	void rtree_stream::rewind()
	{
		i->rewind();
	}

	SpatialIndex::IData* rtree_stream::getNext()
	{
		SpatialObject * so = i->getNext();
		Box mbr = so->getMBR();

		double low[3], high[3];

		low[0] = mbr.low[0];
		low[1] = mbr.low[1];
		low[2] = mbr.low[2];
		high[0] = mbr.high[0];
		high[1] = mbr.high[1];
		high[2] = mbr.high[2];

		SpatialIndex::Region r = SpatialIndex::Region(low, high, 3);

		SpatialIndex::RTree::Data* ret = new SpatialIndex::RTree::Data(0, 0, r, 0);

		delete so;

		return ret;
	};


	PriorityRtreeIndex::PriorityRtreeIndex()
	{
		rtreeStorageManager = NULL;
		tree =  NULL;
	}

	PriorityRtreeIndex::~PriorityRtreeIndex()
	{
		delete rtreeStorageManager;
	}

	void PriorityRtreeIndex::buildIndex(SpatialObjectStream* input,string indexFileStem)
	{
		rtree_stream* ds = new rtree_stream(input);
		SpatialIndex::id_type indexIdentifier=1;
		rtreeStorageManager = SpatialIndex::StorageManager::createNewDiskStorageManager(indexFileStem, PAGE_SIZE);
		try
		{
			uint32 header = 76;
			uint32 overhead = 12;
			uint32 fanout = (uint32)floor(PAGE_SIZE-header+0.0)/(input->objectByteSize+overhead+0.0);

			tree = SpatialIndex::RTree::createAndBulkLoadNewRTree
					(
					SpatialIndex::RTree::BLM_PRI,
					*ds,
					*rtreeStorageManager,
					0.99999, fanout,
					fanout, DIMENSION,
					SpatialIndex::RTree::RV_RSTAR,
					indexIdentifier
					);
		}
		catch (Tools::IllegalArgumentException e)
		{
			cout << e.what() << endl;
		}
		delete ds;
	}

	void PriorityRtreeIndex::loadIndex(string indexFileStem)
	{

	}
}

