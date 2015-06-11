Building Requiments
====================
Cmake Installed
c++ compiler

FLAT Library Dependancies
==========================
Boost Library Installed (1.4, Libraries filesystem thread system) 

RTREE, which can be cloned from: `git@github.com:jutinko/RTREE.git`

1st Step: Compile R-tree library
=================================
```
cd RTREE
make (should create a library libspatialindex.a)
```

2nd Step: Compile FLAT executables
===================================
```
cd FLAT/build
cmake ../cmake -DDEBUG=FALSE -DPRINTRESULT=TRUE
make
```

#########
TEST FLAT
#########

Build FLAT index and query
```
cd FLAT/bin
./FLATG --datafile data.bin --queryfile query.txt
```

###########
TEST R-TREE
###########

Build RTREE index
```
cd FLAT/bin
./RTree --datafile data.bin --queryfile query.txt
```

################
TEST BRUTE FORCE
################
```
3) Query using Sequential Scan 
./BruteTest ../../SampleData.bin ../../SampleQuery.txt
```

############
DATASET INFO
############

Object Type: Vertex
Total Objects: 500000
Object Byte Size: 8
Universe Bounds: Low: (0,0,0)  High: (1000,1000,1000)

Generate more Data:
```
./GenerateRandomData --distribution uniform --elements 50000 --queryRatio 0.01
```
