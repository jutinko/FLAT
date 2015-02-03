#include "ExternalSort.hpp"
#include "Vertex.hpp"
#include <algorithm>

namespace FLAT
{

	ExternalSort::ExternalSort(uint64 footPrintMB, uint8 dim, SpatialObjectType objType)
	{
		objectType = objType;
		objectByteSize = SpatialObjectFactory::getSize(objType);
		objectCount = 0;
		maxObjectsInMemory = footPrintMB*1024*1024/objectByteSize;
		cout << "Max objects in Memory :" << maxObjectsInMemory << endl;
		buffer.reserve(maxObjectsInMemory);
		buckets.clear();
		buffer.clear();
		sortDimension = dim;
		sorted=NULL;
		outOfCore=false;
	}

	ExternalSort::~ExternalSort()
	{
		clean();
	}

	void ExternalSort::rewind()
	{
		if (outOfCore==false)
			iter = buffer.begin();
		else
			sorted->rewindForReading();
	}

	void ExternalSort::makeBucket()
	{

		if (sortDimension==0)
			std::sort(buffer.begin(), buffer.end(), SpatialObjectXAsc());
		else if (sortDimension==1)
			std::sort(buffer.begin(), buffer.end(), SpatialObjectYAsc());
		else std::sort(buffer.begin(), buffer.end(), SpatialObjectZAsc());

		BufferedFile* tf = new BufferedFile();
		tf->maketemporary();

		for (vector<SpatialObject*>::iterator it = buffer.begin(); it!= buffer.end(); ++it)
		{
			tf->write(*it);
			delete (*it);
		}
		buffer.clear();
		tf->rewindForReading();
		buckets.push_back(tf);
		outOfCore=true;

	}

	void ExternalSort::insert(SpatialObject* object)
	{
		cpu.start();
		objectCount++;
		buffer.push_back(object);
		if (buffer.size() >= maxObjectsInMemory)
			makeBucket();
		cpu.stop();
	}

	int ExternalSort::sort()
	{
		cpu.start();
		objectCount=0;
		if (outOfCore==false)
		{
			if (sortDimension==0)
				std::sort(buffer.begin(), buffer.end(), SpatialObjectXAsc());
			else if (sortDimension==1)
				std::sort(buffer.begin(), buffer.end(), SpatialObjectYAsc());
			else std::sort(buffer.begin(), buffer.end(), SpatialObjectZAsc());
			Box::boundingBox(universe,buffer);
			rewind();
			objectCount = buffer.size();
			return buffer.size();
		}

		if (buffer.empty()==false)
			makeBucket();
		cpu.stop();

		disk.start();
		sorted = new BufferedFile();
		sorted->maketemporary();

		uint32 Bsize = 0;
		Bsize = buckets.size();
		buffer.reserve(Bsize);
		uint32 eofCount = 0;
		uint32 outindex = 0;
		/* first object on buffer */
		for (uint32 i = 0; i < Bsize; i++)
		{
			SpatialObject* temp = SpatialObjectFactory::create(objectType);
			buckets.at(i)->read(temp);
			if (buckets.at(i)->eof)
			{
				delete buckets.at(i);
				buckets.at(i) = NULL;
				delete temp;
				temp = NULL;
				eofCount++;
			}
			buffer.push_back(temp);
		}

		while (eofCount < Bsize)
		{
			// set pointer to first item
			for (uint32 i = 0; i < Bsize; i++)
				if (buckets.at(i) != NULL)
				{
					outindex = i;
					break;
				}
			if (outindex == Bsize)
				break;

			// find min item
			for (uint32 i = 0; i < Bsize; i++)
				if (buckets.at(i) != NULL)
					if (buffer.at(i)->getCenter()[sortDimension] < buffer.at(outindex)->getCenter()[sortDimension])
						outindex = i;

			//write Sorted Content.
			sorted->write(buffer.at(outindex));
			objectCount++;
			delete buffer.at(outindex);

			SpatialObject* temp = NULL; temp = SpatialObjectFactory::create(objectType);
			buckets.at(outindex)->read(temp);
			if (buckets.at(outindex)->eof)
			{
				delete buckets.at(outindex);
				buckets.at(outindex) = NULL;
				delete temp;
				temp = NULL;
				eofCount++;
			}
			buffer.at(outindex) = temp;
		}

		for (vector<BufferedFile*>::iterator it = buckets.begin(); it
				!= buckets.end(); ++it)
			delete *it;
		buckets.clear();
		for (vector<SpatialObject*>::iterator it = buffer.begin(); it
				!= buffer.end(); ++it)
			delete *it;
		buffer.clear();
		rewind();
		disk.stop();
		return objectCount;
	}

	SpatialObject* ExternalSort::getNext()
	{
		return next;
	}

	bool ExternalSort::hasNext()
	{
		disk.start();
		if (outOfCore==false)
		{
			if (iter==buffer.end()) {disk.stop();return false;}
			next = (*iter);
			iter++;
			disk.stop();
			return true;
		}
		next = SpatialObjectFactory::create(objectType);
		sorted->read(next);
		if (sorted->eof)
		{
			delete next;
			disk.stop();
			return false;
		}
		disk.stop();
		return true;
	}

	// Reposibility of the caller to deleted Spatial object only if outofcore == true
	SpatialObject* ExternalSort::at(int i)
	{
		if (outOfCore==false)
		{
			return buffer.at(i);
		}
		else
		{
			uint64 currentPointer = sorted->file.tellg();
			uint64 offset = i*objectByteSize;
			sorted->seek(offset);
			SpatialObject* temp = SpatialObjectFactory::create(objectType);
			sorted->read(temp);
			sorted->seek(currentPointer);
			return temp;
		}
	}

	void ExternalSort::clean()
	{
		if (outOfCore==false)
			buffer.clear();

		if (sorted!=NULL)
		{
			delete sorted;
			sorted=NULL;
		}
	}

	void ExternalSort::print()
	{
		cout << "Sort Dimension: " << (int)sortDimension << " CPU Time: " << cpu << " DISK Time: " << disk  << endl;
	}
}
