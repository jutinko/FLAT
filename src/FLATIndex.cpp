#include "FLATIndex.hpp"
#include "ExternalSort.hpp"
#include "SpatialQuery.hpp"
#include "Timer.hpp"
#include "cstdio"

namespace FLAT
{
  class rtreeVisitorFLAT : public SpatialIndex::IVisitor
  {
    public:
      SpatialQuery* query;
      bool done;
      PayLoad* payload;

      rtreeVisitorFLAT(SpatialQuery* query, PayLoad* p)//, string indexFileStem)
      {
        this->query = query;
        done = false;
        payload  = p;
      }

      ~rtreeVisitorFLAT()
      {
      }

      virtual void visitNode(const SpatialIndex::INode& in)
      {
        if (in.isLeaf())
        {
          query->stats.FLAT_metaDataIOs++;
        }
        else
        {
          query->stats.FLAT_seedIOs++;
        }
      }

      virtual void visitData(const SpatialIndex::IData& in)
      {
      }

      virtual void visitUseless()
      {
      }

      virtual bool doneVisiting()
      {
        return done;
      }

      virtual void visitData(const SpatialIndex::IData& in, SpatialIndex::id_type id)
      {
        query->stats.FLAT_metaDataEntryLookup++;
        FLAT::uint8 *b;
        uint32 l;
        in.getData(l, &b);

        MetadataEntry m = MetadataEntry(b, l);
        delete[] b;

        vector<SpatialObject*> so;
        payload->getPageInMemory(so, m.pageId);

        for (vector<SpatialObject*>::iterator it = so.begin(); it != so.end(); ++it)
        {
          if (Box::overlap(query->Region, (*it)->getMBR()))
          {
            done = true;
            query->stats.FLAT_seedId = id;
            break;
          }
        }
      }

      virtual void visitData(std::vector<const SpatialIndex::IData *>& v
          __attribute__((__unused__)))
      {
      }
  };

  class kNNVisitor : public SpatialIndex::IVisitor
  {
    public:
      SpatialQuery* query;
      bool done;

      kNNVisitor(SpatialQuery* query)
      {
        this->query = query;
        done = false;
      }

      ~kNNVisitor()
      {
      }

      virtual void visitNode(const SpatialIndex::INode& in)
      {
        //      cout << "visited node\n";
        //      uint32 children = in.getChildrenCount();
        //
        //      Box Parent;
        //      SpatialIndex::IShape* shape;
        //      in.getShape(&shape);
        //      SpatialIndex::Region reg;
        //      shape->getMBR(reg);
        //      for (int d=0;d<DIMENSION;d++)
        //      {
        //        Parent.low[d] = reg.m_pLow[d];
        //        Parent.high[d] = reg.m_pHigh[d];
        //      }
        //      cout << "PARENT NODE: LEVEL= " << in.getLevel() << " ID= " << in.getIdentifier() << "   MBR= " << Parent << endl;
        //      delete shape;
        //
        //      for (int i=0;i<children;i++)
        //      {
        //        Box Child;
        //        SpatialIndex::IShape* childShape;
        //        in.getChildShape(i,&childShape);
        //        SpatialIndex::Region reg;
        //        childShape->getMBR(reg);
        //        for (int d=0;d<DIMENSION;d++)
        //        {
        //          Child.low[d] = reg.m_pLow[d];
        //          Child.high[d] = reg.m_pHigh[d];
        //        }
        //        cout << "\tCHILD NODE: ID= " << in.getChildIdentifier(i) << "   MBR= " << Child << endl;
        //        delete childShape;
        //      }

        if (in.isLeaf())
        {
          query->stats.FLAT_metaDataIOs++;
        }
        else
        {
          query->stats.FLAT_seedIOs++;
        }
      }

      virtual void visitData(const SpatialIndex::IData& in)
      {
      }

      virtual void visitUseless()
      {
      }

      virtual bool doneVisiting()
      {
        return done;
      }

      virtual void visitData(const SpatialIndex::IData& in, SpatialIndex::id_type id)
      {
        done = true;
        query->stats.FLAT_seedId = id;
      }

      virtual void visitData(std::vector<const SpatialIndex::IData *>& v
          __attribute__((__unused__)))
      {
      }
  };

  class kNNEntry
  {
    public:

      SpatialObject* sobj;
      MetadataEntry* me;
      bigSpaceUnit minDist;
      id metapageId;
      id metaentryId;
      bool isMetaPage;

      kNNEntry(SpatialObject* object,Vertex P,id metaPageId,id metaEntryId)
      {
        sobj = object;
        minDist = sobj->pointDistance(P);
        isMetaPage = false;
        me = NULL;
        metapageId = metaPageId;
        metaentryId = metaEntryId;
      }

      kNNEntry(MetadataEntry* metaEntry,Vertex P,id metaPageId)
      {
        me = metaEntry;
        minDist = me->partitionMbr.pointDistance(P);
        isMetaPage = true;
        sobj = NULL;
        metapageId = metaPageId;
      }

      ~kNNEntry()
      {}

      struct ascending : public std::binary_function<kNNEntry*, kNNEntry*, bool>
    {
      bool operator()(const kNNEntry* __x, const kNNEntry* __y) const
      {
        return __x->minDist > __y->minDist;
      }
    };
  };

  FLATIndex::FLATIndex()
  {
    objectCount=0;
    objectSize=0;
    pageCount=0;
    binCount=0;
    objectPerPage=0;
    objectPerXBins=0;
    objectPerYBins=0;
    footprint=0;
    payload = new PayLoad();
    metadataStructure = new vector<MetadataEntry*>();
  }

  FLATIndex::~FLATIndex()
  {
    delete seedtree;
    delete payload;
    delete rtreeStorageManager;
    delete metadataStructure;
  }

  void FLATIndex::buildIndex(uint64 fp,SpatialObjectStream* input)
  {
    footprint = fp;
    Timer tesselation,seeding,linker;
    tesselation.start();

    initialize(input);
    doTessellation(input);

    tesselation.stop();
    cout << "Tessellation Duration: " << tesselation << "\n";
    linker.start();

    MetaDataStream* metaStream = new MetaDataStream(metadataStructure);
    SpatialIndex::IStorageManager* rtreeStorageManagerTemp = SpatialIndex::StorageManager::createNewMemoryStorageManager();
    uint32 fanout = (uint32)floor(PAGE_SIZE-76+0.0)/(objectSize+12+0.0);

    SpatialIndex::id_type indexIdentifier=1;
    SpatialIndex::ISpatialIndex* linkerTree = SpatialIndex::RTree::createAndBulkLoadNewRTree (
        SpatialIndex::RTree::BLM_STR,
        *metaStream,
        *rtreeStorageManagerTemp,
        0.9999, fanout,
        fanout, DIMENSION,
        SpatialIndex::RTree::RV_RSTAR,
        indexIdentifier);

    linker.stop();
    cout << "Linker Creation Duration: " << linker << "\n";
    seeding.start();

    MetaDataStream* metaDataStream = new MetaDataStream(metadataStructure,linkerTree);
    this->rtreeStorageManager = SeedBuilder::buildSeedTreeInMemory(metaDataStream);

#ifdef DEBUG
    cout << "TOTAL PAGES: " << metaDataStream->pages <<endl;
    cout << "TOTAL LINKS ADDED: " << metaDataStream->links <<endl;
    cout << "SUMMED VOLUME: " << metaDataStream->sumVolume <<endl;
    cout << "AVERAGE LINKS: " << ((metaDataStream->links+0.0) / (metaDataStream->pages+0.0)) << endl;
    cout << "AVERAGE VOLUME: " << ((metaDataStream->sumVolume+0.0) / (metaDataStream->pages+0.0)) << endl;

    //for (int i=0;i<100;i++)
    //  cout << metaDataStream->frequency[i] << "\n";

    //cout << "OVERFLOW VOLUME: " << metaDataStream->overflow <<endl;
    //for (int i=0;i<100;i++)
    //  cout << metaDataStream->volumeDistributon[i] << "\t" << metaDataStream->volumeLink[i] << "\t"
    //    << ( (metaDataStream->volumeLink[i]+0.0)/(metaDataStream->volumeDistributon[i]+0.0)) << "\n" ;
#endif
    delete linkerTree;
    delete metaStream;
    delete metaDataStream;
    delete rtreeStorageManagerTemp;

    seeding.stop();
    cout << "Building Seed Structure & Links Duration: " << seeding << "\n";

  }

  void FLATIndex::initialize(SpatialObjectStream* input)
  {
    objectCount     = input->objectCount;
    objectSize      = input->objectByteSize;
    objectType    = input->objectType;
    universe        = input->universe;
    //Box::infiniteBox(universe); // to make open ended Partition MBRs in the corner of universe... dont do it

#ifdef BBP
    objectPerPage = (uint64)floor((PAGE_SIZE-4.0) / (objectSize+0.0)); // minus 4 bytes because each page has an int counter with it
#else
    objectPerPage = (uint64)floor((PAGE_SIZE-4.0) / (objectSize+0.0));
    //objectPerPage   = 67; // to Degrade FLAT to change back remove comment
#endif
    pageCount       = (uint64)ceil( (objectCount+0.0) / (objectPerPage+0.0) );
    binCount        = pow (pageCount,1.0/(3+0.0));

    objectPerXBins  = (uint64)ceil((objectCount+0.0) / binCount);
    objectPerYBins  = (uint64)ceil((objectPerXBins+0.0) / binCount);

#ifdef DEBUG
    cout << "MINIMUM PAGES NEED TO STORE DATA: "<<pageCount <<endl
      << "PAGES BINS PER DIMENSION: " << binCount << endl
      << "OBJECTS IN EVERY X BIN: " << objectPerXBins << endl
      << "OBJECTS IN EVERY Y BIN: " << objectPerYBins << endl
      << "OBJECTS IN EVERY Z BIN or PAGE: " << objectPerPage << endl;
#endif
    metadataStructure->reserve(pageCount);
    //payload->create(indexFileStem,PAGE_SIZE,objectPerPage,objectSize,objectType);
    payload->createInMemory(objectType);
  }

  void FLATIndex::doTessellation(SpatialObjectStream* input)
  {
    ExternalSort* xSort = new ExternalSort(footprint,0,objectType);
    ExternalSort* ySort = new ExternalSort(footprint,1,objectType);
    ExternalSort* zSort = new ExternalSort(footprint,2,objectType);
    Box Partition = universe;
    uint64 PageCount=0;
    uint64 dataCount=0;

#ifdef DEBUG
    int idx=0,idy=0,idz=0;
#endif
    // SORTING AND BINNING

    while (input->hasNext())
      xSort->insert(input->getNext());

    xSort->sort();

    uint64 xCount=0,oldCountX=0;

    while(xSort->hasNext())
    {
      SpatialObject* xtemp = xSort->getNext();
      ySort->insert(xtemp);
      xCount++;
      if (xCount%objectPerXBins==0 || xCount==objectCount)
      {
        if (xCount>objectPerXBins) Partition.low[0] = Partition.high[0];
        if (xCount==objectCount) Partition.high[0] = universe.high[0];
        else Partition.high[0] = xtemp->getSortDimension(0);
        ySort->sort();
        uint64 yCount=0,oldCountY=0;

        while(ySort->hasNext())
        {
          SpatialObject* ytemp = ySort->getNext();
          zSort->insert(ytemp);
          yCount++;

          if (yCount%objectPerYBins==0 || yCount==xCount-oldCountX)
          {
            if (yCount>objectPerYBins) Partition.low[1] = Partition.high[1];
            if (yCount==xCount-oldCountX) Partition.high[1] = universe.high[1];
            else Partition.high[1] = ytemp->getSortDimension(1);
            zSort->sort();

            /////////////////// MAKING META AND PAYLOAD PAGES ////////////////////
            vector<SpatialObject*> items;
            Box PageMBR;
            uint64 zCount=0;

            while (zSort->hasNext())
            {
              SpatialObject* temp = zSort->getNext();
              //cout << ((Cone*)temp) <<" - " << temp->getSortDimension(0) << " - " << temp->getSortDimension(1) << " - "<< temp->getSortDimension(2) <<endl;
              items.push_back(temp);
              zCount++;
              dataCount++;

              if (zCount%objectPerPage==0 || zCount==yCount-oldCountY)
              {
                Box::boundingBox(PageMBR,items);
                if (zCount>objectPerPage) Partition.low[2] = Partition.high[2];
                if (zCount==yCount-oldCountY) Partition.high[2] = universe.high[2];
                else Partition.high[2] = temp->getSortDimension(2);
                MetadataEntry* metaEntry = new MetadataEntry();
                metaEntry->pageMbr = PageMBR;
                metaEntry->partitionMbr = Partition + PageMBR;
                metaEntry->pageId = PageCount;
#ifdef DEBUG
                metaEntry->i = idx; metaEntry->j = idy; metaEntry->k =idz;
                //cout <<metaEntry->pageId << " ["<< idx << "," << idy << "," << idz << "] \t" << metaEntry->partitionMbr  << "\t" << metaEntry->pageMbr<< endl;
#endif
                metadataStructure->push_back(metaEntry);
                payload->putPageInMemory(PageCount, items);

                PageCount++;
                items.clear();
                if (zCount==yCount-oldCountY) break;
#ifdef DEBUG
                idz++;
#endif
              }

            }
            ////////////////////////////////////////////////////////////////////
            Partition.low[2] = universe.low[2];
            zSort->clean();
            oldCountY = yCount;
            if (yCount==xCount-oldCountX) break;
#ifdef DEBUG
            idy++;idz=0;
#endif
          }
        }
        Partition.low[1] = universe.low[1];
        ySort->clean();
        oldCountX = xCount;
        if (xCount==objectCount) break;
#ifdef DEBUG
        idx++;idy=0;idz=0;
#endif
      }
    }
    Partition.low[0] = universe.low[0];
    xSort->clean();

    cout << "PAGES USED FOR INDEX: " << PageCount << endl;
    cout << "OBJECTS INDEXED: " << dataCount << endl;

    xSort->print();
    ySort->print();
    zSort->print();
    delete zSort;
    delete ySort;
    delete xSort;
  }

  void FLATIndex::induceConnectivityFaster()
  {
    uint32_t pages = metadataStructure->size();
    uint32_t hopFactor = (uint32_t)((floor( (objectPerXBins+0.0) / (objectPerYBins+0.0)) *
          ceil ( (objectPerYBins+0.0) / (objectPerPage+0.0))) +
        ceil ( ((objectPerXBins%objectPerYBins)+0.0) / (objectPerPage+0.0)));
#ifdef DEBUG
    uint32_t links=0;
#endif
    for (uint32_t i=0;i<pages;++i)
    {
      for (uint32_t j=i+1;j<pages;++j)
      {
        if (metadataStructure->at(i)->partitionMbr.high[0] < metadataStructure->at(j)->partitionMbr.low[0])
          break;

        if ((metadataStructure->at(i)->partitionMbr.high[1] < metadataStructure->at(j)->partitionMbr.low[1]))
        {
          uint32_t nextHop = ((j/hopFactor)+1)*hopFactor;
          if (nextHop < pages)
            j = nextHop;
        }

        if (Box::overlap( metadataStructure->at(i)->partitionMbr , metadataStructure->at(j)->partitionMbr ))
        {
          metadataStructure->at(i)->pageLinks.insert(j);
          metadataStructure->at(j)->pageLinks.insert(i);
#ifdef DEBUG
          links+=2;
#endif
        }
      }
#ifdef PROGRESS
      if (i%100000==0 && i>0) cout << "INDUCING LINKS: "<< i << " PAGES DONE" << endl;
#endif
    }

#ifdef DEBUG
    cout << "TOTAL PAGES: " << pages <<endl;
    cout << "TOTAL LINKS ADDED: "<< links <<endl;

    //    int frequency[100];
    //    for (int i=0;i<100;i++) frequency[i]=0;
    //    for (uint32_t j=0;j<pages;j++)
    //    {
    //      if (metadataStructure->at(j)->pageLinks.size()>100)
    //      {
    //        cout << "id("<< j<< ") = [" <<  metadataStructure->at(j)->i << "," << metadataStructure->at(j)->j << "," << metadataStructure->at(j)->k << "] \tLINKS:" << metadataStructure->at(j)->pageLinks.size() << " \tMBR" << metadataStructure->at(j)->partitionMbr << "\n";
    //
    //        //for (set<id>::iterator i = metadataStructure->at(j)->pageLinks.begin();i !=  metadataStructure->at(j)->pageLinks.end(); ++i)
    //        //  cout << "\tid("<< *i << ") = [" <<  metadataStructure->at(*i)->i << "," << metadataStructure->at(*i)->j << "," << metadataStructure->at(*i)->k << "] \tLINKS:" << metadataStructure->at(*i)->pageLinks.size() << " \tMBR" << metadataStructure->at(*i)->partitionMbr << "\n";
    //      }
    //      if (metadataStructure->at(j)->pageLinks.size()<100)
    //        frequency[metadataStructure->at(j)->pageLinks.size()]++;
    //    }


    //    for (int i=0;i<100;i++)
    //      cout << "Links: " << i << " Frequency: " << frequency[i] << "\n";
#endif
  }

  void FLATIndex::query(SpatialQuery *q, vector<SpatialObject *> *result) 
  {
    Timer totalPageReadTimer;
    int queueLength=0;

    Timer intersections;
    Timer queuesearch;
    long intersects = 0;

    q->stats.executionTime.start();
    q->stats.FLAT_seeding.start();
    Timer pageReadTimer;
    queue<int> metapageQueue;
    set<int> visitedMetaPages;

    /*=================== SEEDING ======================*/
    double lo[DIMENSION],hi[DIMENSION];

    for (int i=0;i<DIMENSION;i++)
    {
      lo[i] = (double)q->Region.low[i];
      hi[i] = (double)q->Region.high[i];
    }

    SpatialIndex::Region query_region = SpatialIndex::Region(lo, hi, DIMENSION);
    rtreeVisitorFLAT visitor(&(*q), payload);
    seedtree->seedQuery(query_region, visitor);

    if (q->stats.FLAT_seedId>=0)
    {
      metapageQueue.push(q->stats.FLAT_seedId);
    }

    q->stats.FLAT_seeding.stop();
    q->stats.FLAT_crawling.start();

    unsigned int maxQueueLength = 0;

    /*=================== CRAWLING ======================*/
    while (!metapageQueue.empty())
    {
      /*--------------  VISIT ---------------*/
      queuesearch.start();
      int visitPage = metapageQueue.front();
      metapageQueue.pop();

      bool b1 = visitedMetaPages.find(visitPage) != visitedMetaPages.end();
      queuesearch.stop();

      if(b1) continue;

      pageReadTimer.start();
      nodeSkeleton * nss = SeedBuilder::readNode(visitPage, rtreeStorageManager);
      pageReadTimer.stop();
      queuesearch.start();
      visitedMetaPages.insert(visitPage); queuesearch.stop();
      q->stats.FLAT_metaDataIOs++;

      if (nss->nodeType == SpatialIndex::RTree::PersistentLeaf) //ARE we going to read some non Persistent leaves too??
      {
        for (unsigned i = 0; i < nss->children; i++)
        {
          MetadataEntry m = MetadataEntry(nss->m_pData[i], nss->m_pDataLength[i]);
          for (int a=0;a<DIMENSION;a++)
          {
            m.partitionMbr.low[a]  = nss->m_ptrMBR[i]->m_pLow[a];
            m.partitionMbr.high[a] = nss->m_ptrMBR[i]->m_pHigh[a];
          }
          q->stats.FLAT_metaDataEntryLookup++;

          intersects++;
          intersections.start();
          bool b2 = Box::overlap(m.partitionMbr,q->Region);
          intersections.stop();

          if (b2)
          {
            queuesearch.start();

            for (set<id>::iterator links = m.pageLinks.begin(); links != m.pageLinks.end(); links++)
              metapageQueue.push(*links);

            queuesearch.stop();

            intersects++;
            intersections.start();
            bool b3 = Box::overlap(m.pageMbr,q->Region);
            intersections.stop();

            if (b3)
            {
              vector<SpatialObject*> objects;
              pageReadTimer.start();
              //payload->getPage(objects, m.pageId);
              payload->getPageInMemory(objects, m.pageId);
              pageReadTimer.stop();
              q->stats.FLAT_payLoadIOs++;
              for (vector<SpatialObject*>::iterator it = objects.begin(); it != objects.end(); ++it)
              {
                intersects++;
                intersections.start();
                bool b3 = Box::overlap(q->Region, (*it)->getMBR());
                intersections.stop();
                if (b3) {
                  result->push_back(*it);

                  q->stats.ResultPoints++;

                } else {
                  q->stats.UselessPoints++;
                }

              }
            }
          }
        }
      }
      delete nss;
      if (metapageQueue.size()>maxQueueLength) maxQueueLength=metapageQueue.size();
    }

    q->stats.FLAT_crawling.stop();
    q->stats.executionTime.stop();
    q->stats.printFLATstats();

    queueLength += visitedMetaPages.size()+maxQueueLength;
    totalPageReadTimer.add(pageReadTimer);
  }

  void FLATIndex::kNNQuery(SpatialQuery* query, vector<SpatialObject*>* results)
  {
    set<id> visitedMetaPages;
    priority_queue<kNNEntry*, vector<kNNEntry*>, kNNEntry::ascending> queue;

    double coordinate[DIMENSION];
    for (int i=0;i<DIMENSION;i++)
    {
      coordinate[i] = (double)(query->Point[i]);
    }

    SpatialIndex::Point query_point = SpatialIndex::Point(coordinate, DIMENSION);
    kNNVisitor visitor(query);
    seedtree->seedKNN(query_point, visitor);
    int32 seedID =  query->stats.FLAT_seedId;

    if (seedID>=0)
    {
      // Push seeded Metadata entries in priority queue
      nodeSkeleton* nss = SeedBuilder::readNode(seedID, rtreeStorageManager);
      query->stats.FLAT_metaDataIOs++;
      if (nss->nodeType == SpatialIndex::RTree::PersistentLeaf)
        for (unsigned i = 0; i < nss->children; i++)
        {
          MetadataEntry* m = new MetadataEntry(nss->m_pData[i], nss->m_pDataLength[i]);
          for (int a=0;a<DIMENSION;a++)
          {
            m->partitionMbr.low[a]  = nss->m_ptrMBR[i]->m_pLow[a];
            m->partitionMbr.high[a] = nss->m_ptrMBR[i]->m_pHigh[a];
          }
          queue.push(new kNNEntry(m, query->Point, seedID));
        }
      visitedMetaPages.insert(seedID);
      delete nss;
    }
    else
    {
      //cout << "no seed found.. but thats not possible.. maybe query done outside universe.. solution= make infinite univers bounds" << endl;
      return;
    }

    // Crawling and finding nearest objects //
    uint32 count= 0;
    while (!queue.empty())
    {
      kNNEntry* topEntry = queue.top();
      queue.pop();

      // if the top of PrQueue was a spatial object
      if (topEntry->isMetaPage==false)
      {
        if (results->empty()) count++;
        else if (results->back()->pointDistance(query->Point)!=topEntry->minDist)	count++;
        if (count > query->k) 
        {
          delete topEntry;
          break;
        } else 
        {
          results->push_back(topEntry->sobj);
        }
      } else
      {
        // put all Spatial Objects in page in queue
        vector<SpatialObject*> objects;
        payload->getPageInMemory(objects, topEntry->me->pageId);
        query->stats.FLAT_payLoadIOs++;
        for (vector<SpatialObject*>::iterator it = objects.begin(); it != objects.end(); ++it)
        {
          queue.push(new kNNEntry(*it, query->Point, topEntry->metapageId, topEntry->me->pageId));
        }

        // put all Metadata neighbor entries in queue
        for (set<id>::iterator links = topEntry->me->pageLinks.begin(); links != topEntry->me->pageLinks.end(); links++)
        {
          if(visitedMetaPages.find(*links) != visitedMetaPages.end()) {continue;}
          nodeSkeleton* nss = SeedBuilder::readNode(*links, rtreeStorageManager);
          query->stats.FLAT_metaDataIOs++;
          if (nss->nodeType == SpatialIndex::RTree::PersistentLeaf)
          {
            for (unsigned i = 0; i < nss->children; i++)
            {
              MetadataEntry* m =new MetadataEntry(nss->m_pData[i], nss->m_pDataLength[i]);
              for (int a=0;a<DIMENSION;a++)
              {
                m->partitionMbr.low[a]  = nss->m_ptrMBR[i]->m_pLow[a];
                m->partitionMbr.high[a] = nss->m_ptrMBR[i]->m_pHigh[a];
              }
              queue.push(new kNNEntry(m, query->Point,*links));
            }
          }
          visitedMetaPages.insert(*links);
          delete nss;
        }
        delete topEntry->me;
      }
      delete topEntry;
    }

    // Clean Queue before leaving
    while (! queue.empty())
    {
      kNNEntry* top = queue.top();
      queue.pop();
      if (top->isMetaPage)
        delete top->me;
      delete top;
    }

    query->stats.ResultPoints = results->size();
  }

  void FLATIndex::loadIndex()
  {
    SpatialIndex::id_type indexIdentifier = 1;

    seedtree = SpatialIndex::RTree::loadRTree(*rtreeStorageManager, indexIdentifier);
  }
}
