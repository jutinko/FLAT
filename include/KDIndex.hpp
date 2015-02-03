#ifndef KD_INDEX_HPP
#define KD_INDEX_HPP

#include "Metadata.hpp"
#include "DataFileReader.hpp"
#include "PayLoad.hpp"
#include "SeedBuilder.hpp"

namespace FLAT
{
#define SORTING_FOOTPRINT_MB 400

	/*
	 * Class responsible for managing the Index structure, i.e Metadata, SeedTree and Payload
	 * This is an KD-TREE Style FLAT indexing
	 */
	class KDIndex
	{
	public:
		vector<MetadataEntry*>* metadataStructure;     	// Array of meta data entries leaf Level of Seed Tree
		PayLoad*    	payload;	   		   			// The Structure managing the payload
		uint64 objectCount;
		uint32 objectSize;
		SpatialObjectType objectType;
		uint64 pageCount;
		Box universe;
		uint64 objectPerPage;
		uint64 totalPages;
		uint64 totalIndexed;
		int KSPLIT;

		KDIndex();

		~KDIndex();

		void buildIndex(SpatialObjectStream* input,string indexFileStem,int k);

	private:
		void initialize(SpatialObjectStream* input,string indexFileStem,int k);

		void doTessellation(SpatialObjectStream* input);

		void splitPartition(Box partition,SpatialObjectStream* input, uint64 count);
	};
}
#endif
