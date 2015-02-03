#include "KDIndex.hpp"
#include "ExternalSort.hpp"
#include "Timer.hpp"
#include <math.h>

namespace FLAT
{
	KDIndex::KDIndex()
	{
		objectCount=0;
		objectSize=0;
		pageCount=0;
		objectPerPage=0;
		payload = new PayLoad();
		metadataStructure = new vector<MetadataEntry*>();
		totalPages=0;
		totalIndexed=0;
	}

	KDIndex::~KDIndex()
	{
		delete payload;
		delete metadataStructure;
	}

	void KDIndex::buildIndex(SpatialObjectStream* input,string indexFileStem,int k)
	{
#ifdef PROFILING
		Timer tesselation,seeding,linker;
		tesselation.start();
#endif
#ifdef INFORMATION
		cout << "\n == KD TESSELLATION ==\n\n";
#endif
		initialize(input,indexFileStem,k);
		doTessellation(input);

#ifdef PROFILING
		tesselation.stop();
		cout << "Tessellation Duration: " << tesselation << "\n";
		linker.start();
#endif
#ifdef INFORMATION
		cout << "\n == LINKER RTREE BUILDING ==\n\n";
#endif
		MetaDataStream* metaStream = new MetaDataStream(metadataStructure);
		SpatialIndex::IStorageManager* rtreeStorageManager = SpatialIndex::StorageManager::createNewMemoryStorageManager();
		uint32 fanout = (uint32)floor(PAGE_SIZE-76+0.0)/(objectSize+12+0.0);

		SpatialIndex::id_type indexIdentifier=1;
		SpatialIndex::ISpatialIndex *linkerTree = SpatialIndex::RTree::createAndBulkLoadNewRTree (
		        SpatialIndex::RTree::BLM_STR,
		        *metaStream,
		        *rtreeStorageManager,
				0.9999, fanout,
				fanout, DIMENSION,
		        SpatialIndex::RTree::RV_RSTAR,
		        indexIdentifier);
#ifdef PROFILING
		linker.stop();
		cout << "Linker Creation Duration: " << linker << "\n";
		seeding.start();
#endif
#ifdef INFORMATION
		cout << "\n == BUILDING SEED INDEX WHILE INDUCING LINKS ==\n\n";
#endif
		MetaDataStream* metaDataStream = new MetaDataStream(metadataStructure,linkerTree);

		SeedBuilder::buildSeedTree(indexFileStem,metaDataStream);

#ifdef DEBUG
		cout << "TOTAL PAGES: " << metaDataStream->pages <<endl;
		cout << "TOTAL LINKS ADDED: "<< metaDataStream->links <<endl <<endl;
		for (int i=0;i<100;i++)
			cout << metaDataStream->frequency[i] << "\n";

//		cout << "OVERFLOW VOLUME: " << metaDataStream->overflow <<endl;
//		for (int i=0;i<100;i++)
//			cout << metaDataStream->volumeDistributon[i] << "\t" << metaDataStream->volumeLink[i] << "\t"
//			     << ( (metaDataStream->volumeLink[i]+0.0)/(metaDataStream->volumeDistributon[i]+0.0)) << "\n" ;
#endif
		delete metaDataStream;
#ifdef PROFILING
		seeding.stop();
		cout << "Building Seed KDucture & Links Duration: " << seeding << "\n";
#endif
	}

	void KDIndex::initialize(SpatialObjectStream* input,string indexFileStem,int k)
	{
		objectCount     = input->objectCount;
		objectSize      = input->objectByteSize;
		objectType		= input->objectType;
		universe	    = input->universe;
		KSPLIT = k;

		objectPerPage   = (uint64)floor((PAGE_SIZE-4.0) / (objectSize+0.0)); // minus 4 bytes because each page has an int counter with it
		pageCount       = (uint64)ceil( (objectCount+0.0) / (objectPerPage+0.0) );

#ifdef DEBUG
			cout << "MINIMUM PAGES NEED TO STORE DATA: "<<pageCount <<endl
			 << "TOTAL OBJECTS: " << objectCount << endl;
#endif
		metadataStructure->reserve(pageCount);
		payload->create(indexFileStem,PAGE_SIZE,objectPerPage,objectSize,objectType);
	}

	void KDIndex::doTessellation(SpatialObjectStream* input)
	{
		splitPartition(input->universe,input,input->objectCount);
		cout << "\n\nLARGEST PARTITION SPLIT DONE :" << totalPages << " Indexed: " << totalIndexed << endl;
	}

	void KDIndex::splitPartition(Box binMBR,SpatialObjectStream* input, uint64 count)
	{
		if (count <= objectPerPage)
			{
			// Making MetaData and serializing payload page
			Box PageMBR;
			vector<SpatialObject*> items;

			for (uint64 i=0;i<count;i++)
			{
				if (!input->hasNext())
				{
	#ifdef FATAL
					cout << "Problem: split Partition input stream has ended before expected " << endl;
	#endif
				}
				items.push_back(input->getNext());
			}
			Box::boundingBox(PageMBR,items);

			MetadataEntry* metaEntry = new MetadataEntry();
			metaEntry->pageMbr = PageMBR;
			metaEntry->partitionMbr = binMBR + PageMBR;
			metaEntry->pageId = totalPages;
			totalPages++;

			metadataStructure->push_back(metaEntry);
			payload->putPage(items);
			items.clear();

			totalIndexed +=count;
			return;
			}
		int newDimension = Box::LargestDimension(binMBR);
	    SpatialObjectStream* sorter = new ExternalSort(SORTING_FOOTPRINT_MB,newDimension,objectType);
		while (sorter->objectCount < count)
		{
			if (!input->hasNext())
			{
#ifdef FATAL
				cout << "Problem: split Partition input stream has ended before expected " << endl;
#endif
			}
			((ExternalSort*)sorter)->insert(input->getNext());
		}
		((ExternalSort*)sorter)->sort();
		//int newDimension = (dimension+1) % DIMENSION;  // For cycling partitions

		float ObjectsPerBin = (sorter->objectCount+0.0) / (KSPLIT+0.0);
//		cout << "Sorted on dimension=" << newDimension << " Count=" << count << " Sorting Objects=" << sorter->objectCount << " PartitionMBR=" << partition << endl;

		int splitCount =0,absoluteCount=0;
		Box newPartition = binMBR;

		for (int i=0;i<KSPLIT;i++)
		{
			if (i==KSPLIT-1)
			{
				splitCount = ((ExternalSort*)sorter)->objectCount - ((int)floor(i*ObjectsPerBin)); // because K_SPLITING * ObjectsPerBin != sorter->objectCount
				newPartition.high[newDimension] = binMBR.high[newDimension];
//				for (int j=0;j<level;j++) cout << "\t";
//				cout << "CALL for Last Bin: " << i <<" Total OBJECTS=" << splitCount << " SendMBR=" << newPartition  << "\n";
			}
			else
			{
				splitCount = ((int)floor((i+1)*ObjectsPerBin)) - ((int)floor(i*ObjectsPerBin));
				absoluteCount += splitCount;
				SpatialObject* lastObject = ((ExternalSort*)sorter)->at(absoluteCount-1); // -1 because at() function stores first element at(0)
				newPartition.high[newDimension] = lastObject->getCenter().Vector[newDimension];
				if (((ExternalSort*)sorter)->outOfCore)
					delete lastObject;
//				for (int j=0;j<level;j++) cout << "\t";
//				cout << "CALL for Bin no: " << i <<" Total OBJECTS=" << splitCount  << " Index of LastObject=" << absoluteCount << " SendMBR=" << newPartition  << "\n";
			}
			splitPartition(newPartition,sorter,splitCount);
			newPartition.low[newDimension] = newPartition.high[newDimension];
		}
		delete sorter;
	}
}
