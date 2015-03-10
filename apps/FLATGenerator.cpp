#include "FLATIndex.hpp"
#include "Timer.hpp"

#include <boost/program_options.hpp>

using namespace std;
using namespace FLAT;

namespace po = boost::program_options;

FLATIndex* build(string datafile, uint32 footprint)
{
//  Timer building;
//
//  building.start();
  DataFileReader* input = new DataFileReader(datafile);

  FLATIndex* myIndex = new FLATIndex();
  myIndex->buildIndex(footprint, input);
  myIndex->loadIndex();
  delete input;

  //building.stop();
  //cout << "Building Time: " << building << endl;
  return myIndex;
}

void query(FLATIndex* myIndex, string queryfile)
{
  Timer querying;
  querying.start();

  vector<SpatialQuery> queries;
  SpatialQuery::ReadQueries(queries, queryfile);

  /********************** DO QUERIES **********************/
  for (vector<SpatialQuery>::iterator query = queries.begin(); query != queries.end();query++)
  {
    vector<SpatialObject *> result;
    myIndex->kNNQuery(&(*query), &result);
#ifdef PRINTRESULT
    for(vector<SpatialObject*>::iterator it = result.begin(); it != result.end(); ++it)
    {
      printf("%f ", (*it)->getCenter()[0]);
    }
    printf("\n");
#endif
  }

  querying.stop();
  cout << querying << endl;
}

int main(int argc, const char* argv[])
{
  string datafile, queryfile;
  uint32 footprint = 0;

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

  if(vm.count("help") || argc < 4)
  {
    cout << desc << "\n";
    return 1;
  }
  if(footprint == 0)
  {
    footprint = 2000;
  }

  /********************** BUILDING **********************/
  FLATIndex* myIndex = build(datafile, footprint);

  /********************** DO QUERIES **********************/
  query(myIndex, queryfile);
  delete myIndex;
}
