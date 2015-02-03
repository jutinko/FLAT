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
	string output;

	// Declare the supported options.
	po::options_description desc("Options");
	desc.add_options()("help", "produce help message")("output", po::value<
			string>(&output), "output file name")("distribution", po::value<
			string>(&distribution)->default_value("uniform"),
			"distribution, i.e., uniform or normal")("elements",
			po::value<long>(), "number of elements");

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

	stringstream filename;

	if (output.empty()) {
		if (distribution.compare("normal") == 0) {
			filename << "RandomData-Normal-500-250-" << (elements / 1000)
					<< "K.bin";
		} else {
			filename << "RandomData-Uniform-" << (elements / 1000) << "K.bin";
		}
	} else {
		filename << output;
	}

	BufferedFile* file = new BufferedFile();
	file->create(filename.str());

	//we assume a universe of 0,0,0 and 1000,1000,1000 as well as a resolution of 100 cells in each dimension

	srand( time(NULL));

	double center_x = 0;
	double center_y = 0;
	double center_z = 0;

	int e = 0;

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

		Box random;

		random.low.Vector[0] = center_x - uni_one() / 2;
		random.low.Vector[1] = center_y - uni_one() / 2;
		random.low.Vector[2] = center_z - uni_one() / 2;

		random.high.Vector[0] = center_x + uni_one() / 2;
		random.high.Vector[1] = center_y + uni_one() / 2;
		random.high.Vector[2] = center_z + uni_one() / 2;

		if (universe.low.Vector[0] > random.low.Vector[0])
			universe.low.Vector[0] = random.low.Vector[0];

		if (universe.low.Vector[1] > random.low.Vector[1])
			universe.low.Vector[1] = random.low.Vector[1];

		if (universe.low.Vector[2] > random.low.Vector[2])
			universe.low.Vector[2] = random.low.Vector[2];

		if (universe.high.Vector[0] < random.high.Vector[0])
			universe.high.Vector[0] = random.high.Vector[0];

		if (universe.high.Vector[1] < random.high.Vector[1])
			universe.high.Vector[1] = random.high.Vector[1];

		if (universe.high.Vector[2] < random.high.Vector[2])
			universe.high.Vector[2] = random.high.Vector[2];
		std::cout << random.low[0] << ", " << random.low[1] << ", " << random.low[2] << std::endl;
		std::cout << random.high[0] << ", " << random.high[1] << ", " << random.high[2] << std::endl;
		std::cout << std::endl;

		file->write(&random);
	}

	///////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////

	file->writeUInt32(BOX);
	file->writeUInt64(elements);
	file->writeUInt32(SpatialObjectFactory::getSize(BOX));
	file->write(&universe);

	delete file;
}
