#include <SpatialIndex.h>
#include "SpatialQuery.hpp"
using namespace std;
using namespace FLAT;

class rtreeVisitor : public SpatialIndex::IVisitor
{
public:
	SpatialQuery* query;

    rtreeVisitor(SpatialQuery* query)
	{
		this->query = query;
	}

    ~rtreeVisitor()
    {
    }

    virtual void visitNode(const SpatialIndex::INode& in)
    {
    	if (in.isLeaf())
    	{
    		query->stats.RTREE_leafIOs++;
    	}
    	else
    	{
    		query->stats.RTREE_nodeIOs++;
    	}
	}

    virtual bool doneVisiting()
    {
    	return false;
    }

    virtual void visitData(const SpatialIndex::IData& in)
    {
    	query->stats.ResultPoints++;
    }

    virtual void visitUseless()
    {
    	query->stats.UselessPoints++;
    }

    virtual void visitData(const SpatialIndex::IData& in, SpatialIndex::id_type id)
    {
	}

    virtual void visitData(std::vector<const SpatialIndex::IData *>& v
                            __attribute__((__unused__)))
    {
	}
};

int main(int argc, const char* argv[])
{
	/********************** ARGUMENTS ***********************/

	string inputStem = argv[1];
	string queryFile  = argv[2];

	/******************** LOADING INDEX *********************/

	SpatialIndex::IStorageManager* rtreeStorageManager = SpatialIndex::StorageManager::loadDiskStorageManager(inputStem);
	SpatialIndex::id_type indexIdentifier = 1;
	SpatialIndex::ISpatialIndex* tree = SpatialIndex::RTree::loadRTree(*rtreeStorageManager, indexIdentifier);

	/********************** DO QUERIES **********************/
	vector<SpatialQuery> queries;
	SpatialQuery::ReadQueries(queries,queryFile);

	QueryStatistics totalStats;
	uint32 fanout = (uint32)floor(PAGE_SIZE-76+0.0)/(48+12+0.0);

	totalStats.ObjectSize = SpatialObjectFactory::getSize(BOX);
	totalStats.ObjectsPerPage = fanout;
	//totalStats.printRTREEheader();

	for (vector<SpatialQuery>::iterator query = queries.begin(); query != queries.end(); query++)
	{
		query->stats.ObjectSize = SpatialObjectFactory::getSize(BOX);
		query->stats.ObjectsPerPage = fanout;
#ifndef WIN32
		system("sync");
		system("echo 1 > /proc/sys/vm/drop_caches");
		system("dd if=/dev/zero of=deleteME bs=1M count=128");
		system("sync");
		system("cat deleteME > /dev/null");
#endif
		query->stats.executionTime.start();
		double lo[DIMENSION], hi[DIMENSION];
		for (int i = 0; i < DIMENSION; i++)
		{
			lo[i] = (double) (*query).Region.low[i];
			hi[i] = (double) (*query).Region.high[i];
		}

		SpatialIndex::Region query_region = SpatialIndex::Region(lo, hi, DIMENSION);
		rtreeVisitor visitor(&(*query));
		tree->intersectsWithQuery(query_region, visitor);
		query->stats.executionTime.stop();

		query->stats.printRTREEstats();
		totalStats.add(query->stats);
	}
	totalStats.printRTREEstats();
}
