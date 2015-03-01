#include "FLATIndex.hpp"
#include "Timer.hpp"

#include <boost/program_options.hpp>

using namespace std;
using namespace FLAT;

namespace po = boost::program_options;

int main(int argc, const char* argv[])
{
///////////////////////// TEST STR INDEX AND TESSELLATION ////////////////////////////

	string datafile, queryfile;
	uint32 footprint;

	po::options_description desc("Options");
	desc.add_options()
			("help", "produce help message")
			("datafile", po::value<string>(&datafile), "file containing the data to be indexed")
			("footprint", po::value<uint32>(&footprint), "maximum memory footprint of indexing process")
			("queryfile", po::value<string>(&queryfile), "file containing the queries");
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help") || argc < 4) {
		cout << desc << "\n";
		return 1;
	}

	Timer building;

	building.start();
	DataFileReader* input = new DataFileReader(datafile);
	cout << "heyo: " << endl;

	FLATIndex* myIndex = new FLATIndex();
	myIndex->buildIndex(footprint, input);
  myIndex->loadIndex();

	building.stop();
	delete input;
	cout << "Building Time: " << building << endl;

	/********************** DO QUERIES **********************/
	vector<SpatialQuery> queries;
	SpatialQuery::ReadQueries(queries, queryfile);

	for (vector<SpatialQuery>::iterator query = queries.begin(); query != queries.end();query++) {
		vector<SpatialObject *> result;
		myIndex->query(&(*query), &result);
    vector<SpatialObject*>::iterator it;
	}
  delete myIndex;
}
