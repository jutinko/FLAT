#ifndef STR_INDEX_HPP
#define STR_INDEX_HPP

#include "Metadata.hpp"
#include "DataFileReader.hpp"
#include "PayLoad.hpp"
#include "SpatialQuery.hpp"
#include "SeedBuilder.hpp"

namespace FLAT
{
  /*
   * Class responsible for managing the Index structure, i.e Metadata, SeedTree and Payload
   * This is an STR Style FLAT indexing
   */
  class FLATIndex
  {
    public:
      vector<MetadataEntry*>* metadataStructure;     	// Array of meta data entries leaf Level of Seed Tree
      PayLoad*    	payload;	   		   			// The Structure managing the payload
      SpatialIndex::ISpatialIndex* seedtree;
      SpatialIndex::IStorageManager* rtreeStorageManager;

      uint64 objectCount;
      uint32 objectSize;
      SpatialObjectType objectType;
      uint64 pageCount;
      Box universe;
      float binCount;
      uint64 objectPerPage;
      uint64 objectPerXBins;
      uint64 objectPerYBins;
      uint64 footprint;

      FLATIndex();

      ~FLATIndex();

      void buildIndex(uint64 fp,SpatialObjectStream* input);

      void loadIndex();

      void query(SpatialQuery *qi, vector<SpatialObject *>* result);

      void kNNQuery(SpatialQuery* q, vector<SpatialObject*>* result);

    private:
      void initialize(SpatialObjectStream* input);

      void doTessellation(SpatialObjectStream* input);

      void induceConnectivity();

      void induceConnectivityFaster();

  };
}
#endif
