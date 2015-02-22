#include <SpatialIndex.h>
#include "SpatialQuery.hpp"
#include "RtreeIndex.hpp"

#include <boost/program_options.hpp>

using namespace std;
using namespace FLAT;

namespace po = boost::program_options;

int main(int argc, const char* argv[]) {

	//string inputStem, queryfile;
	string queryfile;
	string datafile;

	po::options_description desc("Options");
	desc.add_options()
			("help", "produce help message")
			("datafile", po::value<string>(&datafile), "name fordatafile")
			("queryfile", po::value<string>(&queryfile), "file containing the queries");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help") || argc < 2) {
		cout << desc << "\n";
		return 1;
	}

	/********************** BUILDING **********************/
	Timer building;
	building.start();
	RtreeIndex* myIndex = new RtreeIndex();
	DataFileReader* input = new DataFileReader(datafile);

	myIndex->buildIndex(input);

	delete input;
	building.stop();
	cout << "Building Time: " << building << endl;
	
  myIndex->setObjectType(VERTEX);
	//rtree->loadIndex(inputStem, BOX);

	/********************** DO QUERIES **********************/
	vector<SpatialQuery> queries;
	SpatialQuery::ReadQueries(queries, queryfile);

	for(vector<SpatialQuery>::iterator query = queries.begin(); query != queries.end(); query++) {
		vector<SpatialObject *> result;
		myIndex->kNNQuery(&(*query), &result);
    //query->stats.printRTREEstats();

    for(vector<SpatialObject*>::iterator i = result.begin(); i != result.end(); ++i)
    {
      delete *i;
    }
	}

  delete myIndex;
}
