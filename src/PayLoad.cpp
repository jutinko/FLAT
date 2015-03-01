#include "PayLoad.hpp"
#include "Box.hpp"
#include <iostream>
namespace FLAT
{
  PayLoad::PayLoad()
  {
    file = NULL;
  }

  PayLoad::~PayLoad()
  {
    if(file != NULL)
    {
      delete file;
    }
    unordered_map<uint64, vector<SpatialObject*> >::iterator it;
    for(it = table.begin(); it != table.end(); ++it)
    {
      vector<SpatialObject*>::iterator it2;
      for(it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      {
        delete *it2;
      }
    }
  }

  void PayLoad::create(string indexFileStem,uint32 pageSize,uint64 objectsPerPage,uint32 objectSize,SpatialObjectType objectType)
  {
    try
    {
      this->filename     = indexFileStem + "_payload.dat";
      this->pageSize       = pageSize;
      this->objType      = objectType;
      this->objectsPerPage = objectsPerPage;
      this->objectSize     = objectSize;
      this->isCreated      = true;

      file  = new BufferedFile();
      file->create(this->filename);

      int8 page[pageSize];
      for (uint32 i=0;i<pageSize;i++) page[i]=0;
      int8* ptr = page;

      memcpy(ptr,&pageSize,sizeof(uint32));
      ptr+=sizeof(uint32);
      uint32 type = objType;
      memcpy(ptr,&type,sizeof(uint32));
      ptr+=sizeof(uint32);
      memcpy(ptr,&objectsPerPage,sizeof(uint64));
      ptr+=sizeof(uint64);
      memcpy(ptr,&objectSize,sizeof(uint32));

      file->write(pageSize,page);
    }
    catch(...)
    {
#ifdef FATAL
      cout << "Cannot create Payload File: " << this->filename << endl;
#endif
      exit(0);
    }

  }
  
  void PayLoad::createInMemory(SpatialObjectType objectType)
  {
    this->objType = objectType;
  }

  void PayLoad::load(string indexFileStem)
  {
    try
    {
      this->filename = indexFileStem + "_payload.dat";
      file  = new BufferedFile();
      file->open(this->filename);

      this->pageSize = file->readUInt32();
      this->objType  =  (SpatialObjectType)file->readUInt32();
      this->objectsPerPage = file->readUInt64();
      this->objectSize = file->readUInt32();

      this->isCreated= false;
    }
    catch(...)
    {
#ifdef FATAL
      cout << "Cannot load Payload File: " << this->filename << endl;
#endif
      exit(0);
    }

  }

  // This method only works when we are writing the items to file
  bool PayLoad::putPage(vector<SpatialObject*>& itemArray)
  {
    try
    {
      if (!isCreated) return false;

      // Write counter how many vertices in page to differentiate between a partially filled page
      int8 page[pageSize];
      int8*ptr = page;
      for (uint32 i=0;i<pageSize;i++) page[i]=0;

      uint32 items = itemArray.size();
      uint32 objectByteSize = SpatialObjectFactory::getSize(objType);
      memcpy(ptr,&items,sizeof(uint32));
      ptr+=sizeof(uint32);

      for (vector<SpatialObject*>::iterator it = itemArray.begin();it !=itemArray.end();++it)
        {
          (*it)->serialize(ptr);
          ptr+=objectByteSize;
          delete (*it);
        }

      file->write(pageSize,page);
    }
    catch(...)
    {
      cout << "problem writing page: ";
      return false;
    }
    return true;
  }

  bool PayLoad::putPageInMemory(uint64 id, vector<SpatialObject*>& itemArray)
  {
    if(table.find(id) != table.end()) 
    {
      return false;
    } else 
    {
      vector<SpatialObject*> items;
      for(size_t i = 0; i < itemArray.size(); ++i)
      {
        FLAT::Box* b = new FLAT::Box();
        *b = *(dynamic_cast<FLAT::Box *>(itemArray[i]));

        delete itemArray[i];
        items.push_back(b);
      }

      table.insert(make_pair(id, items));
      return true;
    }
  }

  // This method only works when we reading thigns from file
  bool PayLoad::getPage(vector<SpatialObject*>& itemArray,int pageId)
  {
    try
    {
      if (isCreated) return false;

      int8 page[pageSize];
      int8* ptr = page;

      uint64_t offset = (uint64_t)(pageId+1)*(uint64_t)pageSize; // +1 cause first page is Header

      file->seek(offset);
      file->read(pageSize,page);

      uint32 counter=0;
      uint32 objectByteSize = SpatialObjectFactory::getSize(objType);

      memcpy(&counter,ptr,sizeof(uint32));
      ptr += sizeof(uint32);

      for (uint32 i=0;i<counter;i++)
      {
        SpatialObject* sobj = SpatialObjectFactory::create(objType);
        sobj->unserialize(ptr);
        ptr+=objectByteSize;
        itemArray.push_back(sobj);
      }
    }
    catch(...)
    {
      cout << "problem reading page: " <<pageId << "\n";
      return false;
    }
    return true;
  }

  bool PayLoad::getPageInMemory(vector<SpatialObject*>& itemArray, uint64 pageId)
  {
    if(table.find(pageId) == table.end())
    {
      return false;
    } else
    {
      vector<SpatialObject*> objects = (*table.find(pageId)).second;
      for(vector<SpatialObject*>::iterator i = objects.begin(); i != objects.end(); ++i)
      {
        itemArray.push_back(*i);
      }
      return true;
    }
  }
}
