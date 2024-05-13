#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <cstring>
#include <chrono>
#include <cmath>
#include "SignedRandomProjection.h"
#include "LSH.h"
#include "LSH_Ourlier.h"

using namespace std;

bool is_power_of_two(int n){
    return (n > 0) && ((n & (n-1)) == 0);
}

void hadamard_transform(double* work, int N, double* output) 
{
	int i, j, k, stage, L, J, M;
	double *tmp;
	for (i = 0; i < N-1; i += 2)
	{
		work[i] = (work[i] + work[i+1]);
		work[i+1] = (work[i] - 2*work[i+1]);
	}
	L = 1;
	for (stage = 2; stage <= ceil(log2(N)); ++stage)
	{
		M = (int) pow((double)2, L);
		J = 0;
		k = 0;

		while (k < N-1)
		{
			for (j = J; j < J+M-1; j = j+2)
			{
				output[k] = work[j] + work[j+M];
				output[k+1] = work[j] - work[j+M];
				output[k+2] = work[j+1] - work[j+M+1];
				output[k+3] = work[j+1] + work[j+M+1];
				k += 4;
			}
			J += 2*M;
		}
		tmp = work;
		work = output;
		output = tmp;
		L += 1;
	}
}

bool *lsh(vector<double *> &data, int numData, int dim, double *time, int K, int L, double alpha) {

    // auto start_time = chrono::high_resolution_clock::now();
    double total_query_time = 0.0;

    //Create HashTables
    LSH *_Algo = new LSH(K, L, K);
    SignedRandomProjection *proj = new SignedRandomProjection(dim, K * L);

    double Hashtime = 0.0;
    for (int i = 0; i < numData; i++) {
        auto t1 = chrono::high_resolution_clock::now();
        
        //*********ACE and FastACE
        int *hashes = proj->getHash(data[i], dim);

        //*********ACHashACE
        // int *hashes;
        // if(is_power_of_two(dim)){
        //     double firHT[dim];
        //     double *tmp = data[i]; 
        //     hadamard_transform(tmp, dim, firHT);
        //     hashes = proj->getHash(tmp, dim);
        // }else{
        //     int dimension = pow(2, ceil(log2(dim)));
        //     double firHT[dimension];
        //     double tmp[dimension] = {0}; 
        //     for (int d = 0; d < dim; ++d){tmp[d] = data[i][d];}
        //     hadamard_transform(tmp, dimension, firHT);
        //     hashes = proj->getHash(tmp, dim);  
        // }

        auto t2 = std::chrono::high_resolution_clock::now();
        float timeMiliseconds = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        Hashtime += timeMiliseconds;
        _Algo->add(hashes, i);
    }
    
    std::cout<<"Hashing time is " << Hashtime / 1000000 << " (s)"<< std::endl;

    double mean = 0;
    double stdev = 0;

    double *localmean = new double[numData];
#pragma omp parallel for
    for (int i = 0; i < numData; i++) {
        auto start_time = chrono::high_resolution_clock::now();
        
        //********ACE and FastACE
        int *hashes = proj->getHash(data[i], dim);
        localmean[i] = _Algo->retrieve_mean(hashes);

        //********ACHashACE
        // int *hashes;
        // if(is_power_of_two(dim)){
        //     double firHT[dim];
        //     double *tmp = data[i]; 
        //     hadamard_transform(tmp, dim, firHT);
        //     hashes = proj->getHash(tmp, dim);
        // }else{
        //     int dimension = pow(2, ceil(log2(dim)));
        //     double firHT[dimension];
        //     double tmp[dimension] = {0}; 
        //     for (int d = 0; d < dim; ++d){tmp[d] = data[i][d];}
        //     hadamard_transform(tmp, dimension, firHT);
        //     hashes = proj->getHash(tmp, dim);
        // }
        double query_time = (chrono::high_resolution_clock::now() - start_time).count() * 1e-9;
        total_query_time += query_time;
        localmean[i] = _Algo->retrieve_mean(hashes);
    }

    *time = total_query_time;



    for (size_t i = 0; i < numData; i++) {
        mean += localmean[i];
    }
    mean = mean / (double) numData;
    for (int i = 0; i < numData; i++) {
        stdev += (localmean[i] - mean) * (localmean[i] - mean);
    }
    stdev = sqrt(stdev / (numData - 1));

    double threshold = mean - alpha * stdev;

    bool *outlier = new bool[numData];

#pragma omp parallel for
    for (int i = 0; i < numData; i++) {
        outlier[i] = localmean[i] < threshold;
    }

    delete _Algo;
    delete proj;
    delete[]localmean;
    // *time = (chrono::high_resolution_clock::now() - start_time).count() * 1e-9;

    return outlier;
}

std::vector<double *> load(string filename, int dimension) {
    ifstream file(filename);
    string str;
    std::vector<double *> data;
    while (getline(file, str)) {
        double *row = new double[dimension];
        int i = 0;
        std::stringstream stream(str);
        double d;
        while (stream >> d) {
            row[i++] = d;
        }
        if (i != dimension) {
            cout << "ERROR: dimension doesn't match." << endl;
            exit(1);
        }
        data.push_back(row);
    }
    return data;
}
