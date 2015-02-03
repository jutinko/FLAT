#include "RtreeIndex.hpp"
#include "Timer.hpp"
#include <boost/program_options.hpp>

using namespace std;
using namespace FLAT;

namespace po = boost::program_options;

int main(int argc, const char* argv[]) {

	string outputStem, datafile;

	po::options_description desc("Options");
	desc.add_options()
			("help", "produce help message")
			("indexname", po::value<string>(&outputStem), "stem name of the index files")
			("datafile", po::value<string>(&datafile), "file containing the data to be indexed");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help") || argc < 2) {
		cout << desc << "\n";
		return 1;
	}

	Timer building;
	building.start();
	RtreeIndex* myIndex = new RtreeIndex();
	DataFileReader* input = new DataFileReader(datafile);

	myIndex->buildIndex(input, outputStem);

	delete input;
	delete myIndex;
	building.stop();
	cout << "Building Time: " << building << endl;
}
