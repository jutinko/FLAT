#include "FLATIndex.hpp"
#include "Timer.hpp"

#include <boost/program_options.hpp>

using namespace std;
using namespace FLAT;

namespace po = boost::program_options;

int main(int argc, const char* argv[])
{
///////////////////////// TEST STR INDEX AND TESSELLATION ////////////////////////////

	string outputStem, datafile;
	uint32 footprint;

	po::options_description desc("Options");
	desc.add_options()
			("help", "produce help message")
			("indexname", po::value<string>(&outputStem), "stem name of the index files")
			("datafile", po::value<string>(&datafile), "file containing the data to be indexed")
			("footprint", po::value<uint32>(&footprint), "maximum memory footprint of indexing process")
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

	FLATIndex* myIndex = new FLATIndex();
	myIndex->buildIndex(footprint, input, outputStem);

	delete myIndex;
	delete input;
	building.stop();
	cout << "Building Time: " << building << endl;
}
