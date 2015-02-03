#ifndef METADATA_
#define METADATA_

#include "Box.hpp"
#include <SpatialIndex.h>
#include <set>

using namespace std;
namespace FLAT
{

typedef uint32_t id;
/*
 * if the Metadata cannot fit in memory for 500M data points put on disk
 * Projected for 500Miliion points size of Metadata = 200Mb at most
 */
	class MetadataEntry
	{
	public:
		Box pageMbr;
		Box partitionMbr;
		set<id> pageLinks;
		id pageId;
#ifdef DEBUG
		int i,j,k;

#endif

		MetadataEntry();

		MetadataEntry(byte * buffer, int length);

		~MetadataEntry();

		void serialize(byte ** buffer, int &length);

		void print();

	private:

		void write(spaceUnit f, int pos, byte* buf);

		void writeId(int i, int pos, byte* buf);

		int readId(int pos, byte* buf);

		spaceUnit read(int pos, byte* buf);
	};


	class MetaVisitor : public SpatialIndex::IVisitor
	{
	public:
		vector<MetadataEntry*>* metadataStructure;
		int i;
		MetaVisitor(vector<MetadataEntry*>* metadataStructure,int i);
	    ~MetaVisitor();
		virtual void visitNode(const SpatialIndex::INode& in);
		virtual void visitData(const SpatialIndex::IData& in);
		virtual void visitUseless();
		virtual void visitData(const SpatialIndex::IData& in, SpatialIndex::id_type);
		virtual void visitData(std::vector<const SpatialIndex::IData*>& v);
		virtual bool doneVisiting();
	};

	class MetaDataStream : public SpatialIndex::IDataStream
	{
	public:
		unsigned i;
		uint32_t pages;
		bool dolinking;
		SpatialIndex::ISpatialIndex *linkerTree;

#ifdef DEBUG
		uint32_t links;
		int frequency[100];
		bigSpaceUnit sumVolume;
	//	int volumeDistributon[100];
	//	int volumeLink[100];
	//	int overflow;
#endif
		vector<MetadataEntry*>* metadataStructure;

		MetaDataStream (vector<MetadataEntry*>* metadataStructure,SpatialIndex::ISpatialIndex *linkerTree);
		MetaDataStream (vector<MetadataEntry*>* metadataStructure);

		virtual ~MetaDataStream();

		virtual bool hasNext();

		virtual uint32_t size();

		virtual void rewind();

		virtual SpatialIndex::IData* getNext();

		void GenerateLinks(MetadataEntry* me,uint32_t i);
	};

}

#endif
