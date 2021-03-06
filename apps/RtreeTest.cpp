#include <SpatialIndex.h>
#include "SpatialQuery.hpp"
#include "RtreeIndex.hpp"

#include <boost/program_options.hpp>

using namespace std;
using namespace FLAT;

namespace po = boost::program_options;

RtreeIndex* build(string datafile)
{
//	Timer building;
//	building.start();
	RtreeIndex* myIndex = new RtreeIndex();
	DataFileReader* input = new DataFileReader(datafile);

	myIndex->buildIndex(input);

	delete input;
  return myIndex;
//	building.stop();
//	cout << "Building Time: " << building << endl;
  
}

void query(RtreeIndex* myIndex, string queryfile, bool print)
{
	Timer querying;
	querying.start();

	/********************** DO QUERIES **********************/
	vector<SpatialQuery> queries;
	SpatialQuery::ReadQueries(queries, queryfile);

	for(vector<SpatialQuery>::iterator query = queries.begin(); query != queries.end(); query++) 
  {
		vector<SpatialObject *> result;
		myIndex->kNNQuery(&(*query), &result);
    if(print)
    {
      for(vector<SpatialObject*>::iterator it = result.begin(); it != result.end(); ++it)
      {
        printf("%f ", (*it)->getCenter()[0]);
      }
      printf("\n");
    } 	
  }
  if(!print)
  {
    cout << querying;
  }

}

int main(int argc, const char* argv[])
{

	//string inputStem, queryfile;
	string queryfile;
	string datafile;
  bool print;

	po::options_description desc("Options");
	desc.add_options()
			("help", "produce help message")
			("datafile", po::value<string>(&datafile), "name fordatafile")
			("queryfile", po::value<string>(&queryfile), "file containing the queries")
      ("print", po::value<bool>(&print), "if print result");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help") || argc < 2)
  {
		cout << desc << "\n";
		return 1;
	}

	/********************** BUILDING **********************/
	
  RtreeIndex* myIndex = build(datafile);
  myIndex->setObjectType(VERTEX);

	/********************** QUERYING **********************/

  query(myIndex, queryfile, print);

  delete myIndex;
}
