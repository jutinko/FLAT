#include "DataFileReader.hpp"
#include "SpatialQuery.hpp"

using namespace FLAT;
using namespace std;

int main(int argc, const char* argv[])
{
	if (argc<3) {cout << "Insufficient Parameters" << endl;exit(0);}

	string dataFile  = argv[1];
	string queryFile  = argv[2];
    DataFileReader* input = new DataFileReader(dataFile);

	vector<SpatialQuery> queries;
	SpatialQuery::ReadQueries(queries,queryFile);
	QueryStatistics totalStats;

	for (vector<SpatialQuery>::iterator query = queries.begin(); query != queries.end();query++)
	{
		query->stats.executionTime.start();
		while (input->hasNext())
		{
			SpatialObject* object = input->getNext();
			if (Box::overlap(query->Region, object->getMBR()))
				query->stats.ResultPoints++;
			delete object;
		}
		input->rewind();
		query->stats.executionTime.stop();

		query->stats.printBRUTEstats();
		totalStats.add(query->stats);
	}
	cout <<"\n";
	totalStats.printBRUTEstats();
	delete input;
}
