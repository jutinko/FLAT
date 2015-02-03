#include "SpatialQuery.hpp"
#include "FLATIndex.hpp"

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


	FLATIndex *fi = new FLATIndex();
	fi->loadIndex(inputStem);
	/********************** DO QUERIES **********************/
	vector<SpatialQuery> queries;
	SpatialQuery::ReadQueries(queries, queryfile);

	for (vector<SpatialQuery>::iterator query = queries.begin(); query != queries.end();query++) {
		vector<SpatialObject *> * result = new vector<SpatialObject *>();
		fi->query(&(*query), result);
		
		delete result;
	}
}

