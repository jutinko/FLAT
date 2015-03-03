#include <boost/program_options.hpp>

#include <boost/random.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/variate_generator.hpp>

#include "DataFileReader.hpp"
#include "Box.hpp"

namespace po = boost::program_options;

using namespace std;
using namespace FLAT;

int main(int argc, char* argv[]) {
	string distribution;
	string dataFile, queryFile;
  double ratio;

	// Declare the supported options.
	po::options_description desc("Options");
	desc.add_options()("help", "produce help message")("dataFile", po::value<
			string>(&dataFile), "dataFile file name")("queryFile", po::value<
			string>(&queryFile), "queryFile file name")("distribution", po::value<
			string>(&distribution)->default_value("uniform"),
			"distribution, i.e., uniform or normal")("elements",
			po::value<long>(), "number of elements")
      ("queryRatio", po::value<double>(&ratio), "the ratio between queries and data");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help") || argc < 2) {
		cout << desc << "\n";
		return 1;
	}

	Box universe;

	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	long elements = vm["elements"].as<long> ();

	static boost::mt19937 generator(time(0));
	boost::normal_distribution<> normal_dist(500, 250);
	boost::variate_generator<boost::mt19937 &, boost::normal_distribution<> >
			norm(generator, normal_dist);

	boost::uniform_real<> uni_dist(0, 1000);
	boost::variate_generator<boost::mt19937 &, boost::uniform_real<> > uni(
			generator, uni_dist);

	boost::uniform_real<> uni_dist_one(0, 1);
	boost::variate_generator<boost::mt19937 &, boost::uniform_real<> > uni_one(
			generator, uni_dist_one);

	cout << distribution << " " << elements << endl;

	stringstream dataFilename, queryFilename;

	if (dataFile.empty()) {
		if (distribution.compare("normal") == 0) {
			dataFilename << "RandomData-Normal-500-250-" << (elements/1000)
					<< "K.bin";
		} else {
			dataFilename << "RandomData-Uniform-" << (elements/1000) << "K.bin";
		}
	} else {
		dataFilename << dataFile;
	}

	if (queryFile.empty()) {
		if (distribution.compare("normal") == 0) {
			queryFilename << "RandomQuery-Normal-500-250-" << (elements/1000)
					<< "K.bin";
		} else {
			queryFilename << "RandomQuery-Uniform-" << (elements*ratio/1000) << "K.txt";
		}
	} else {
		queryFilename << queryFile;
	}

	BufferedFile* dFile = new BufferedFile();
	dFile->create(dataFilename.str());

	ofstream qFile;
	qFile.open(queryFilename.str());
  qFile << "3\n";
  qFile << "3\n";

	//we assume a universe of 0,0,0 and 1000,1000,1000 as well as a resolution of 100 cells in each dimension

	srand( time(NULL));

	double center_x = 0;
	double center_y = 0;
	double center_z = 0;

	int e = 0;
  int querySize = elements*ratio;

	while (e < elements) {

		if (distribution.compare("normal") == 0) {
			center_x = ((int) norm());
			center_y = ((int) norm());
			center_z = ((int) norm());

			if(center_x > 1000 || center_y > 1000 || center_z > 1000 || center_x < 0 || center_y < 0 || center_z < 0) continue;

		} else {
			center_x = uni();
			center_y = uni();
			center_z = uni();
		}

		e++;

		//cout << center_x << "\t" << center_y << "\t" << center_z << endl;

		//Box random;
    Vertex random;

		random.Vector[0] = center_x + uni_one() / 2;
		random.Vector[1] = center_y + uni_one() / 2;
		random.Vector[2] = center_z + uni_one() / 2;

		if (universe.high.Vector[0] < random.Vector[0])
			universe.high.Vector[0] = random.Vector[0];

		if (universe.high.Vector[1] < random.Vector[1])
			universe.high.Vector[1] = random.Vector[1];

		if (universe.high.Vector[2] < random.Vector[2])
			universe.high.Vector[2] = random.Vector[2];

    if(querySize)
    {
      qFile << random[0] << " " << random[1] << " " << random[2] << endl;
      --querySize;
    }

		dFile->write(&random);
	}
  qFile.close();

	///////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////

	dFile->writeUInt32(VERTEX);
	dFile->writeUInt64(elements);
	dFile->writeUInt32(SpatialObjectFactory::getSize(VERTEX));
	dFile->write(&universe);

	delete dFile;
}
