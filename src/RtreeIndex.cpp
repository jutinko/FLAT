#include "RtreeIndex.hpp"
#include "SpatialQuery.hpp"

namespace FLAT
{

	class rtreeVisitorRTREE : public SpatialIndex::IVisitor {

	public:
		SpatialQuery * query;
		vector<SpatialObject *> *result;
		SpatialObjectType type;

		rtreeVisitorRTREE(SpatialQuery* query, vector<SpatialObject *> *r, SpatialObjectType t) {
			this->query = query;
			this->result = r;
			this->type = t;
		}

		~rtreeVisitorRTREE() {
		}

		virtual void visitNode(const SpatialIndex::INode& in) {
			SpatialIndex::IShape* shape;
			in.getShape(&shape);

			if (in.isLeaf()) {
				query->stats.RTREE_leafIOs++;
			} else {
				query->stats.RTREE_nodeIOs++;
			}
      delete shape;
		}

		virtual bool doneVisiting() {
			return false;
		}

		virtual void visitData(const SpatialIndex::IData& in) {

			query->stats.ResultPoints++;

			byte *b;
			uint32_t l;
			in.getData(l, &b);

			SpatialObject* data = SpatialObjectFactory::create(type);

			data->unserialize((int8*)b);

			result->push_back(data);
      delete b;
		}

		virtual void visitUseless() {
			query->stats.UselessPoints++;
		}

		virtual void visitData(const SpatialIndex::IData& in, SpatialIndex::id_type id) {
		}

		virtual void visitData(std::vector<const SpatialIndex::IData *>& v __attribute__((__unused__))) {
		}
	};

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

		uint32 objectByteSize = SpatialObjectFactory::getSize(so->getType());
		int8 page[objectByteSize];
		int8* ptr = page;

		so->serialize(ptr);

		byte * s = reinterpret_cast<byte*>(ptr);

		SpatialIndex::RTree::Data* ret = new SpatialIndex::RTree::Data((int)objectByteSize, s, r, 0);

		delete so;

		return ret;
	};


	RtreeIndex::RtreeIndex() {
		rtreeStorageManager = NULL;
		tree =  NULL;
	}

	RtreeIndex::~RtreeIndex() {
    delete tree;
		delete rtreeStorageManager;
	}

	void RtreeIndex::buildIndex(SpatialObjectStream* input) {
		rtree_stream* ds = new rtree_stream(input);
		SpatialIndex::id_type indexIdentifier=1;
		//rtreeStorageManager = SpatialIndex::StorageManager::createNewDiskStorageManager(indexFileStem, PAGE_SIZE);
    rtreeStorageManager = SpatialIndex::StorageManager::createNewMemoryStorageManager();

		try {
			uint32 header = 76;
			uint32 overhead = 12;
			uint32 fanout = (uint32)floor(PAGE_SIZE-header+0.0)/(input->objectByteSize+overhead+0.0);

			tree = SpatialIndex::RTree::createAndBulkLoadNewRTree
					(
					SpatialIndex::RTree::BLM_STR,
					*ds,
					*rtreeStorageManager,
					0.99999, fanout,
					fanout, DIMENSION,
					SpatialIndex::RTree::RV_RSTAR,
					indexIdentifier
					);

		} catch (Tools::IllegalArgumentException e) {
			cout << e.what() << endl;
		}

		delete ds;
	}

	void RtreeIndex::loadIndex(string indexFileStem, SpatialObjectType type) {

		rtreeStorageManager = SpatialIndex::StorageManager::loadDiskStorageManager(indexFileStem);
		SpatialIndex::id_type indexIdentifier = 1;
		tree = SpatialIndex::RTree::loadRTree(*rtreeStorageManager, indexIdentifier);
		objectType = type;
	}

	void RtreeIndex::query(SpatialQuery *q, vector<SpatialObject *> *result) {

		double lo[DIMENSION], hi[DIMENSION];

		for (int i=0;i<DIMENSION;i++) {
			lo[i] = (double)q->Region.low[i];
			hi[i] = (double)q->Region.high[i];
		}

		SpatialIndex::Region query_region = SpatialIndex::Region(lo, hi, DIMENSION);
		rtreeVisitorRTREE visitor(q, result, objectType);

		tree->intersectsWithQuery(query_region, visitor);
	}

	void RtreeIndex::kNNQuery(SpatialQuery *q, vector<SpatialObject *> *result) {

    double values[DIMENSION];

		for (int i=0;i<DIMENSION;i++) {
			values[i] = (double)q->Point.Vector[i];
		}

    SpatialIndex::Point p = SpatialIndex::Point(values, DIMENSION);
		rtreeVisitorRTREE visitor(q, result, objectType);

		tree->nearestNeighborQuery(q->k, p, visitor);
	}

	void RtreeIndex::setObjectType(SpatialObjectType type) {
    objectType = type;
  }
}

