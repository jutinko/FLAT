#include <SpatialIndex.h>
#include "SpatialQuery.hpp"
#include "RtreeIndex.hpp"

#include <boost/program_options.hpp>

using namespace std;
using namespace FLAT;

namespace po = boost::program_options;

int main(int argc, const char* argv[]) {

	string inputStem, queryfile;

	po::options_description desc("Options");
	desc.add_options()
			("help", "produce help message")
			("indexname", po::value<string>(&inputStem), "stem name of the index files")
			("queryfile", po::value<string>(&queryfile), "file containing the queries");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help") || argc < 2) {
		cout << desc << "\n";
		return 1;
	}

	RtreeIndex *rtree = new RtreeIndex();
	rtree->loadIndex(inputStem, BOX);

	/********************** DO QUERIES **********************/
	vector<SpatialQuery> queries;
	SpatialQuery::ReadQueries(queries, queryfile);

	for (vector<SpatialQuery>::iterator query = queries.begin(); query != queries.end(); query++) {

//#ifndef WIN32
//		system("sync");
//		system("echo 1 > /proc/sys/vm/drop_caches");
//		system("dd if=/dev/zero of=deleteME bs=1M count=128");
//		system("sync");
//		system("cat deleteME > /dev/null");
//#endif
		vector<SpatialObject *> * result = new vector<SpatialObject *>();
		rtree->query(&(*query), result);
    query->stats.printRTREEstats();

		delete result;
	}
}
