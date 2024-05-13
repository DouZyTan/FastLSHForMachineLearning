/*
 *   Copyright (c) 2004-2005 Massachusetts Institute of Technology.
 *   All Rights Reserved.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *   Authors: Alexandr Andoni (andoni@mit.edu), Piotr Indyk (indyk@mit.edu)
*/

/*
  The main entry file containing the main() function. The main()
  function parses the command line parameters and depending on them
  calls the correspondin functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/times.h>
#include "headers.h"

#include <iostream>
#include <fstream>
#include <cstdio>
#include <queue>
#include <algorithm>
#include <cmath>
#include <vector>

#define N_SAMPLE_QUERY_POINTS 100

// The data set containing all the points.
PPointT *dataSetPoints = NULL;
// Number of points in the data set.
IntT nPoints = 0;
// The dimension of the points.
IntT pointsDimension = 0;
// The value of parameter R (a near neighbor of a point <q> is any
// point <p> from the data set that is the within distance
// <thresholdR>).
//RealT thresholdR = 1.0;

// The succes probability of each point (each near neighbor is
// reported by the algorithm with probability <successProbability>).
RealT successProbability = 0.9;

// Same as <thresholdR>, only an array of R's (for the case when
// multiple R's are specified).
RealT *listOfRadii = NULL;
IntT nRadii = 0;

RealT *memRatiosForNNStructs = NULL;

char sBuffer[600000];

/*
  Prints the usage of the LSHMain.
 */
void usage(char *programName){
  printf("Usage: %s #pts_in_data_set #queries dimension successProbability radius data_set_file query_points_file max_available_memory [-c|-p params_file]\n", programName);
}

  template<typename T>
  void readXvec(std::ifstream &in, T *data, const size_t d, const size_t n = 1)
  {
      uint32_t dim = d;
      for (size_t i = 0; i < n; i++) {
          in.read((char *) &dim, sizeof(uint32_t));
          if (dim != d) {
              std::cout << "file error\n";
              exit(1);
          }
          in.read((char *) (data + i * dim), dim * sizeof(T));
      }
  }
  
void load_ivecs_data(
  const char* filename, 
  std::vector<std::vector<unsigned>>& results, 
  unsigned &num, unsigned &dim)
{
  std::ifstream in(filename, std::ios::binary);
  if(!in.is_open()){
    std::cout<<"open file error"<<std::endl;
    exit(-1);
  }
  
  in.read((char*)&dim, 4);
  in.seekg(0, std::ios::end);
  std::ios::pos_type ss = in.tellg();
  size_t fsize = (size_t)ss;
  num = (unsigned)(fsize / (dim + 1) / 4);
  results.resize(num);
  for (unsigned i = 0; i < num; ++i){
    results[i].resize(dim);
  }
  in.seekg(0, std::ios::beg);
  for(size_t i = 0; i < num; ++i){
    in.seekg(4, std::ios::cur);
    in.read((char*)results[i].data(), dim * 4);
  }
  in.close();
}

void compute_recall(
  std::vector<unsigned> Qtrue, 
  IntT index,  
  unsigned &count,
  unsigned max)
{
  for(size_t i = 0; i < max; ++i){
    if((int)index == Qtrue[i]){count += 1;}
  }
}

inline PPointT readPoint(FILE *fileHandle)
{
  PPointT p;
  RealT sqrLength = 0;
  FAILIF(NULL == (p = (PPointT)MALLOC(sizeof(PointT))));
  FAILIF(NULL == (p->coordinates = (RealT*)MALLOC(pointsDimension * sizeof(RealT))));
  for(IntT d = 0; d < pointsDimension; d++){
    FSCANF_REAL(fileHandle, &(p->coordinates[d]));
    sqrLength += SQR(p->coordinates[d]);
  }
  fscanf(fileHandle, "%[^\n]", sBuffer);
  p->index = -1;
  p->sqrLength = sqrLength;
  return p;
}

// Reads in the data set points from <filename> in the array
// <dataSetPoints>. Each point get a unique number in the field
// <index> to be easily indentifiable.
void readDataSetFromFile(char *filename)
{
  FILE *f = fopen(filename, "rt");
  FAILIF(f == NULL);
  
  //fscanf(f, "%d %d ", &nPoints, &pointsDimension);
  //FSCANF_DOUBLE(f, &thresholdR);
  //FSCANF_DOUBLE(f, &successProbability);
  //fscanf(f, "\n");

  FAILIF(NULL == (dataSetPoints = (PPointT*)MALLOC(nPoints * sizeof(PPointT))));
  
  for(IntT i = 0; i < nPoints; i++){
    dataSetPoints[i] = readPoint(f);
    dataSetPoints[i]->index = i;
  }
}

// Tranforming <memRatiosForNNStructs> from
// <memRatiosForNNStructs[i]=ratio of mem/total mem> to
// <memRatiosForNNStructs[i]=ratio of mem/mem left for structs i,i+1,...>.
void transformMemRatios()
{
  RealT sum = 0;
  for(IntT i = nRadii - 1; i >= 0; i--){
    sum += memRatiosForNNStructs[i];
    memRatiosForNNStructs[i] = memRatiosForNNStructs[i] / sum;
    //DPRINTF("%0.6lf\n", memRatiosForNNStructs[i]);
  }
  ASSERT(sum <= 1.000001);
}


int compareInt32T(const void *a, const void *b)
{
  Int32T *x = (Int32T*)a;
  Int32T *y = (Int32T*)b;
  return (*x > *y) - (*x < *y);
}







/*
  The main entry to LSH package. Depending on the command line
  parameters, the function computes the R-NN data structure optimal
  parameters and/or construct the R-NN data structure and runs the
  queries on the data structure.
 */




int main(int nargs, char **args)
{
  if(nargs < 9){
    usage(args[0]);
    exit(1);
  }
  

  //initializeLSHGlobal();

  // Parse part of the command-line parameters.
  nPoints = atoi(args[1]);
  IntT nQueries = atoi(args[2]);
  pointsDimension = atoi(args[3]);
  successProbability = atof(args[4]);
  char* endPtr[1];
  RealT thresholdR = strtod(args[5], endPtr);   
  int subdim = 30;
  // int subdim = ceil(0.25 * pointsDimension);
  std::cout<<pointsDimension<<' '<<subdim<<' '<<nQueries<<' ' <<nPoints<<std::endl;



  if (thresholdR == 0 || endPtr[1] == args[5]){
    // The value for R is not specified, instead there is a file
    // specifying multiple R's.
    thresholdR = 0;

    // Read in the file
    FILE *radiiFile = fopen(args[5], "rt");
    FAILIF(radiiFile == NULL);
    fscanf(radiiFile, "%d\n", &nRadii);
    ASSERT(nRadii > 0);
    FAILIF(NULL == (listOfRadii = (RealT*)MALLOC(nRadii * sizeof(RealT))));
    FAILIF(NULL == (memRatiosForNNStructs = (RealT*)MALLOC(nRadii * sizeof(RealT))));
    for(IntT i = 0; i < nRadii; i++){
      FSCANF_REAL(radiiFile, &listOfRadii[i]);
      ASSERT(listOfRadii[i] > 0);
      FSCANF_REAL(radiiFile, &memRatiosForNNStructs[i]);
      ASSERT(memRatiosForNNStructs[i] > 0);
    }
  }else{
    nRadii = 1;
    FAILIF(NULL == (listOfRadii = (RealT*)MALLOC(nRadii * sizeof(RealT))));
    FAILIF(NULL == (memRatiosForNNStructs = (RealT*)MALLOC(nRadii * sizeof(RealT))));
    listOfRadii[0] = thresholdR;
    memRatiosForNNStructs[0] = 1;
  }
  DPRINTF("No. radii: %d\n", nRadii);
  //thresholdR = atof(args[5]);
  availableTotalMemory = atoll(args[8]);

  if (nPoints > MAX_N_POINTS) {
    printf("Error: the structure supports at most %d points (%d were specified).\n", MAX_N_POINTS, nPoints);
    fprintf(ERROR_OUTPUT, "Error: the structure supports at most %d points (%d were specified).\n", MAX_N_POINTS, nPoints);
    exit(1);
  }

  //read data file from command
  readDataSetFromFile(args[6]);
  DPRINTF("Allocated memory (after reading data set): %lld\n", totalAllocatedMemory);

  // printf("data has been read \n");




  RNNParametersT *algParameters = NULL;
  PRNearNeighborStructT *nnStructs = NULL;
  if (nargs > 9) {

    // Additional command-line parameter is specified.
    if (strcmp("-c", args[9]) == 0) {

      // printf("Operation NULL");

      // printf("%s",args[9]);
      // //Only compute the R-NN DS parameters and output them to stdout.
      
      // printf("%d\n", nRadii);
      // transformMemRatios();
      // for(IntT i = 0; i < nRadii; i++){
	    //   //which sample queries to use
	    //   Int32T segregatedQStart = (i == 0) ? 0 : sampleQBoundaryIndeces[i - 1];
	    //   Int32T segregatedQNumber = nSampleQueries - segregatedQStart;
	    //   if (segregatedQNumber == 0) {
	    //     // XXX: not the right answer
	    //     segregatedQNumber = nSampleQueries;
	    //    segregatedQStart = 0;
	    //   }
	    //   ASSERT(segregatedQStart < nSampleQueries);
	    //   ASSERT(segregatedQStart >= 0);
	    //   ASSERT(segregatedQStart + segregatedQNumber <= nSampleQueries);
	    //   ASSERT(segregatedQNumber >= 0);


      //   //wrong in computing optimal parameters
	    //   RNNParametersT optParameters = computeOptimalParameters(listOfRadii[i],
			// 					successProbability,
			// 					nPoints,
			// 					pointsDimension,
			// 					dataSetPoints,
			// 					segregatedQNumber,
			// 					sampleQueries + segregatedQStart,
			// 					(MemVarT)((availableTotalMemory - totalAllocatedMemory) * memRatiosForNNStructs[i]));
	    //   printRNNParameters(stdout, optParameters);
      // }

      // exit(0);


    } else if (strcmp("-p", args[9]) == 0) {
        // Read the R-NN DS parameters from the given file and run the
        // queries on the constructed data structure.
        if (nargs < 10){
	        usage(args[0]);
	        exit(1);
        }
        FILE *pFile = fopen(args[10], "rt");
        FAILIFWR(pFile == NULL, "Could not open the params file.");
        fscanf(pFile, "%d\n", &nRadii);
        DPRINTF1("Using the following R-NN DS parameters:\n");
        DPRINTF("N radii = %d\n", nRadii);
        FAILIF(NULL == (nnStructs = (PRNearNeighborStructT*)MALLOC(nRadii * sizeof(PRNearNeighborStructT))));
        FAILIF(NULL == (algParameters = (RNNParametersT*)MALLOC(nRadii * sizeof(RNNParametersT))));
        for(IntT i = 0; i < nRadii; i++){
	        algParameters[i] = readRNNParameters(pFile);
	        printRNNParameters(stderr, algParameters[i]);

          clock_t start, end;
          start = clock();
	        // nnStructs[i] = initLSH_WithDataSet(algParameters[i], nPoints, dataSetPoints); // E2LSH

          // nnStructs[i] = FinitLSH_WithDataSet(algParameters[i], nPoints, dataSetPoints, subdim); // ACHash
          
          nnStructs[i] = RinitLSH_WithDataSet(algParameters[i], nPoints, dataSetPoints, subdim); // FastLSH
          
          end = clock();
          std::cout<<"Indexing time is "<<(double)(end-start) / CLOCKS_PER_SEC <<"(s)"<<std::endl;
        }

        pointsDimension = algParameters[0].dimension;
        FREE(listOfRadii);
        FAILIF(NULL == (listOfRadii = (RealT*)MALLOC(nRadii * sizeof(RealT))));
        for(IntT i = 0; i < nRadii; i++){
	        listOfRadii[i] = algParameters[i].parameterR;
        }
    } else{
        // Wrong option.
        usage(args[0]);
        exit(1);
      }
  } else {
      FAILIF(NULL == (nnStructs = (PRNearNeighborStructT*)MALLOC(nRadii * sizeof(PRNearNeighborStructT))));
      // Determine the R-NN DS parameters, construct the DS and run the queries.
      transformMemRatios();
      for(IntT i = 0; i < nRadii; i++){
        //printf("search Radii %d is ", listOfRadii[i]);
        //XXX: segregate the sample queries...
      //   nnStructs[i] = initSelfTunedRNearNeighborWithDataSet(listOfRadii[i], 
			// 				   successProbability, 
			// 				   nPoints, 
			// 				   pointsDimension, 
			// 				   dataSetPoints, 
			// 				   nSampleQueries, 
			// 				   sampleQueries, 
			// 				   (MemVarT)((availableTotalMemory - totalAllocatedMemory) * memRatiosForNNStructs[i]));
      }
    }
    
    nQueries = 200;
    std::vector<std::vector<unsigned>> true_load;
    unsigned dim = 100;
    unsigned Qnum = nQueries;
  
    load_ivecs_data(args[11], true_load, Qnum, dim);
    

    DPRINTF1("X\n");

    IntT resultSize = nPoints;
    PPointT *result = (PPointT*)MALLOC(resultSize * sizeof(*result));
    PPointT queryPoint;
    FAILIF(NULL == (queryPoint = (PPointT)MALLOC(sizeof(PointT))));
    FAILIF(NULL == (queryPoint->coordinates = (RealT*)MALLOC(pointsDimension * sizeof(RealT))));


    // PPointT query;
    // FAILIF(NULL == (query = (PPointT)MALLOC(sizeof(PointT))));
    // FAILIF(NULL == (query->coordinates = (RealT*)MALLOC(subdim * sizeof(RealT))));


    FILE *queryFile = fopen(args[7], "rt");
    FAILIF(queryFile == NULL);
    TimeVarT meanQueryTime = 0;
    PPointAndRealTStructT *distToNN = NULL;

    std::vector<float> Qrecall;
    Qrecall.resize(nQueries);

    int TotalPoints = 0;

    for(IntT i = 0; i < 200; i++){

      unsigned count = 0;
      unsigned nMax = 10;

      RealT sqrLength = 0;
      for(IntT d = 0; d < pointsDimension; d++){
        FSCANF_REAL(queryFile, &(queryPoint->coordinates[d]));
        sqrLength += SQR(queryPoint->coordinates[d]);
      }
      queryPoint->sqrLength = sqrLength;

      // get the near neighbors.
      int num = 0;
      IntT nNNs = 0;
      for(IntT r = 0; r < nRadii; r++){
        // nNNs = getRNearNeighbors(nnStructs[r], queryPoint, result, resultSize, num); // E2LSH

        // nNNs = FgetRNearNeighbors(nnStructs[r], queryPoint, result, resultSize, num, subdim); // ACHash
        
        nNNs = R2getRNearNeighbors(nnStructs[r], queryPoint, result, resultSize, num, subdim); // FastLSH
    
        // printf("Total time for R-NN query at radius %0.6lf (radius no. %d):\t%0.6lf\n", (double)(listOfRadii[r]), r, timeRNNQuery);
        meanQueryTime += timeRNNQuery;

        if (nNNs > 0){
	        // printf("Query point %d: found %d NNs at distance %0.6lf (%dth radius). First %d NNs are:\n", i, nNNs, (double)(listOfRadii[r]), r, MIN(nNNs, MAX_REPORTED_POINTS));
	
	        // compute the distances to the found NN, and sort according to the distance
	        FAILIF(NULL == (distToNN = (PPointAndRealTStructT*)REALLOC(distToNN, nNNs * sizeof(*distToNN))));
	        for(IntT p = 0; p < nNNs; p++){
	          distToNN[p].ppoint = result[p];
	          distToNN[p].real = distance(pointsDimension, queryPoint, result[p]);
	        }
	        qsort(distToNN, nNNs, sizeof(*distToNN), comparePPointAndRealTStructT);

	        // Print the points
	        for(IntT j = 0; j < MIN(nNNs, MAX_REPORTED_POINTS); j++){
	          ASSERT(distToNN[j].ppoint != NULL);

            compute_recall(true_load[i], distToNN[j].ppoint->index, count, nMax);

	          // printf("%09d\tDistance:%0.6lf\n", distToNN[j].ppoint->index, distToNN[j].real);
	          CR_ASSERT(distToNN[j].real <= listOfRadii[r]);
	          //DPRINTF("Distance: %lf\n", distance(pointsDimension, queryPoint, result[j]));
	          //printRealVector("NN: ", pointsDimension, result[j]->coordinates);
	        }
	        break;
        }
      }
      TotalPoints += num;

      Qrecall[i] = (float)count / 10;
      

      if (nNNs == 0){
        //printf("Query point %d: no NNs found.\n", i);
      }

    }
    printf("\n");    
    float recall = 0;
    int ratio = 0;
    for(int q = 0; q < 200; ++q){
      recall += Qrecall[q];
    }
    printf("recall is %f\n", (recall / 200));
    printf("ratio of access point is %lf\n", (float)TotalPoints / (200 * nPoints));



    if (nQueries > 0){
      meanQueryTime = meanQueryTime / nQueries;
      printf("Mean query time: %0.6lf\n", (double)meanQueryTime);
    }

    for(IntT i = 0; i < nRadii; i++){
      freePRNearNeighborStruct(nnStructs[i]);
    }
    // XXX: should ideally free the other stuff as well.


  return 0;
}
