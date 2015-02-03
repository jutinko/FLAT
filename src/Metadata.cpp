#include "Metadata.hpp"
#include "Box.hpp"
#include <set>

using namespace std;
namespace FLAT {

	MetadataEntry::MetadataEntry()
	{
	}

	MetadataEntry::MetadataEntry(byte * buffer, int length)
	{
		pageMbr.low[0]  = read(0*sizeof(spaceUnit), buffer);
		pageMbr.low[1]  = read(1*sizeof(spaceUnit), buffer);
		pageMbr.low[2]  = read(2*sizeof(spaceUnit), buffer);
		pageMbr.high[0] = read(3*sizeof(spaceUnit), buffer);
		pageMbr.high[1] = read(4*sizeof(spaceUnit), buffer);
		pageMbr.high[2] = read(5*sizeof(spaceUnit), buffer);

		pageId = readId(6*sizeof(spaceUnit), buffer);
		int count = readId(6*sizeof(spaceUnit) + sizeof(id), buffer);

		for (int i=0; i<count; i++)
		{
			pageLinks.insert(readId(6*sizeof(spaceUnit) + 2*sizeof(id) + i*sizeof(id), buffer));
		}
	}

	MetadataEntry::~MetadataEntry()
	{
		pageLinks.clear();
	}

	void MetadataEntry::serialize(byte ** buffer, int &length)
	{
		//12 float for pageMBR and partitionMbr
		//size of unsigned int times pageLinks
		length = sizeof(spaceUnit) * 6 + sizeof(spaceUnit) + sizeof(id) + sizeof(id) * pageLinks.size();

		*buffer = new byte[length];

		// Major Change from V1.0 seed tree built on partition MBR now so in buffer we keep pageMbr

		write(pageMbr.low[0],  0*sizeof(spaceUnit), *buffer);
		write(pageMbr.low[1],  1*sizeof(spaceUnit), *buffer);
		write(pageMbr.low[2],  2*sizeof(spaceUnit), *buffer);
		write(pageMbr.high[0], 3*sizeof(spaceUnit), *buffer);
		write(pageMbr.high[1], 4*sizeof(spaceUnit), *buffer);
		write(pageMbr.high[2], 5*sizeof(spaceUnit), *buffer);

		writeId(pageId, 6*sizeof(spaceUnit), *buffer);
		writeId((int)pageLinks.size(), 6*sizeof(spaceUnit) + sizeof(id), *buffer);

		int i=0;

		for (set<id>::iterator it = pageLinks.begin();it!=pageLinks.end();it++)
		{
			writeId((id)*it, 6*sizeof(spaceUnit) + 2*sizeof(id) + i*sizeof(id), *buffer);
			i++;
		}
	}

	void MetadataEntry::print()
	{
		cout << "Page ("<< pageLinks.size() << ") -> ";
		for (set<id>::iterator it = pageLinks.begin();it!=pageLinks.end();it++)
			cout << *it << " ";
		cout << "\n";
	}

	void MetadataEntry::write(spaceUnit f, int pos, byte* buf)
	{
		byte* tmp = (byte*)&f;

		for(unsigned i=0; i<sizeof(spaceUnit); i++)
		{
			buf[i+pos] = tmp[i];
		}
	}

	void MetadataEntry::writeId(int i, int pos, byte* buf)
	{
		byte* tmp = (byte*)&i;

		for(unsigned j=0; j<sizeof(id); j++)
		{
			buf[j+pos] = tmp[j];
		}
	}

	int MetadataEntry::readId(int pos, byte* buf)
	{
		byte tmp[sizeof(id)];

		for(unsigned i=0; i<sizeof(id); i++) {
			tmp[i] = buf[i+pos];
		}

		int val = *reinterpret_cast<int *>(tmp);

		return val;
	}

	spaceUnit MetadataEntry::read(int pos, byte* buf)
	{
		byte tmp[sizeof(spaceUnit)];

		for(unsigned i=0; i<sizeof(spaceUnit); i++)
		{
			tmp[i] = buf[i+pos];
		}

		spaceUnit val = *reinterpret_cast<spaceUnit *>(tmp);

		return val;
	}



	MetaVisitor::MetaVisitor(vector<MetadataEntry*>* metadataStructure,int i)
	{
		this->metadataStructure = metadataStructure;
		this->i=i;
	}
	MetaVisitor::~MetaVisitor()
    {
    }
	void MetaVisitor::visitNode(const SpatialIndex::INode& in)
	{
	}
	void MetaVisitor::visitData(const SpatialIndex::IData& in)
	{
		SpatialIndex::id_type id = in.getIdentifier();
		if (i==id) return;
		metadataStructure->at(i)->pageLinks.insert(id);
	}
	void MetaVisitor::visitUseless()
	{

	}
	void MetaVisitor::visitData(const SpatialIndex::IData& in, SpatialIndex::id_type)
	{

	}
	void MetaVisitor::visitData(std::vector<const SpatialIndex::IData*>& v)
	{

	}
	bool MetaVisitor::doneVisiting()
	{
		return false;
	}




	MetaDataStream::MetaDataStream (vector<MetadataEntry*>* metadataStructure,SpatialIndex::ISpatialIndex *linkerTree)
	{
		i=0;
		this->linkerTree = linkerTree;
		this->metadataStructure = metadataStructure;
		pages = metadataStructure->size();
#ifdef DEBUG
		links=0;
		sumVolume=0;
		for (int i=0;i<100;i++)
		{
//			volumeDistributon[i]=0;
//			volumeLink[i]=0;
			frequency[i]=0;
		}
//		overflow=0;

#endif
		dolinking=true;
	}

	MetaDataStream::MetaDataStream (vector<MetadataEntry*>* metadataStructure)
	{
		i=0;
		this->metadataStructure = metadataStructure;
		pages = metadataStructure->size();
#ifdef DEBUG
		links=0;
		sumVolume=0;
		for (int i=0;i<100;i++)
			frequency[i]=0;
#endif
		dolinking=false;
	}

	MetaDataStream::~MetaDataStream()
	{
	}

	bool MetaDataStream::hasNext()
	{
		return (i < pages);
	}

	uint32_t MetaDataStream::size()
	{
		return pages;
	}

	void MetaDataStream::rewind()
	{
		i=0;
	}

	SpatialIndex::IData* MetaDataStream::getNext()
	{
		if (dolinking)
		{
			MetadataEntry* me = metadataStructure->at(i);

			/********** INDUCE LINKS BY RTREE ***************/
			double lo[DIMENSION], hi[DIMENSION];
			for (int k = 0; k < DIMENSION; k++)
			{
				lo[k] = (double) me->partitionMbr.low[k];
				hi[k] = (double) me->partitionMbr.high[k];
			}

			SpatialIndex::Region query_region = SpatialIndex::Region(lo, hi, DIMENSION);
			MetaVisitor visitor(metadataStructure,i);
			linkerTree->intersectsWithQuery(query_region, visitor);
			
			//cout << "For MetaData Entry ID: " << i<< " Links= " << me->pageLinks.size() << endl;
	#ifdef DEBUG
			if (metadataStructure->at(i)->pageLinks.size()<100)
				frequency[metadataStructure->at(i)->pageLinks.size()]++;
			links += metadataStructure->at(i)->pageLinks.size();

			bigSpaceUnit volume  = Box::volume(metadataStructure->at(i)->partitionMbr);
			sumVolume+= volume;
//			uint32 index = (uint32)volume/1000;
//			if (index>=100)
//				{
//				cout << volume << "\t" << metadataStructure->at(i)->pageLinks.size() << "\n";
//				overflow++;
//				}
//			else
//				{
//				volumeDistributon[index]++;
//				volumeLink[index]+=metadataStructure->at(i)->pageLinks.size();
//				}
	#endif

			if (me->pageId != i) cout << "Something wrong";

			byte * buffer;
			int length;
			me->serialize(&buffer, length);

			double low[3], high[3];

			// MAJOR change from V 1.0 Seed tree was built on pageMBR first .. now Partition MBR

			low[0] = me->partitionMbr.low[0];
			low[1] = me->partitionMbr.low[1];
			low[2] = me->partitionMbr.low[2];
			high[0] = me->partitionMbr.high[0];
			high[1] = me->partitionMbr.high[1];
			high[2] = me->partitionMbr.high[2];

			SpatialIndex::Region r = SpatialIndex::Region(low, high, 3);
			SpatialIndex::RTree::Data* ret = new SpatialIndex::RTree::Data(length, buffer, r, i);
			i++;
	#ifdef PROGRESS
				if (i%100000==0) cout << "INDUCING LINKS: "<< i << " PAGES DONE" << endl;
	#endif
			delete[] buffer;
			delete me;
			return ret;
		}
		else
		{
			MetadataEntry* me = metadataStructure->at(i);
			if (me->pageId != i) cout << "Something wrong";
			double low[3], high[3];
			low[0] = me->partitionMbr.low[0];
			low[1] = me->partitionMbr.low[1];
			low[2] = me->partitionMbr.low[2];
			high[0] = me->partitionMbr.high[0];
			high[1] = me->partitionMbr.high[1];
			high[2] = me->partitionMbr.high[2];

			SpatialIndex::Region r = SpatialIndex::Region(low, high, 3);
			SpatialIndex::RTree::Data* ret = new SpatialIndex::RTree::Data(0, 0, r, i);
			i++;
			return ret;
		}
	}



}

