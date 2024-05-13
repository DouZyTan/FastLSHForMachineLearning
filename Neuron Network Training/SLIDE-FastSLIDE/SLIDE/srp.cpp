#include "srp.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>

using namespace std;

SparseRandomProjection::SparseRandomProjection(size_t dimension, size_t numOfHashes, int ratio) {
    _dim = dimension;
    _numhashes = numOfHashes;
    _samSize = ceil(0.07 * _dim); // this is the key to select dimenisons for SLIDE and FastSLIDE
    printf("%ld %ld\n", _dim, _samSize);
    int *a = new int[_dim];
    for (size_t i = 0; i < _dim; i++) {
        a[i] = i;
    }

    srand(time(0));
    _randBits = new float *[_numhashes];
    _indices = new int *[_numhashes];

    std::random_device rd;
    std::mt19937 e2(rd());
    std::normal_distribution<> dist(0,1);

    for (size_t i = 0; i < _numhashes; i++) {
        random_shuffle(a, a+_dim);
        _randBits[i] = new float[_samSize];
        _indices[i] = new int[_samSize];
        for (size_t j = 0; j < _samSize; j++) {
            _indices[i][j] = a[j];
            _randBits[i][j] = dist(e2);
        }
        std::sort(_indices[i], _indices[i]+_samSize);
    }
    delete [] a;
}


int *SparseRandomProjection::getHash(float *vector, int length) {
    int *hashes = new int[_numhashes];

//  #pragma omp parallel for
    for (size_t i = 0; i < _numhashes; i++) {
        double s = 0;
        for (size_t j = 0; j < _samSize; j++) {
            float v = vector[_indices[i][j]];
            s += v * _randBits[i][j];
        }
        hashes[i] = (s >= 0 ? 0 : 1);
    }
    return hashes;
}


int *SparseRandomProjection::getHashSparse(int* indices, float *values, size_t length) {
    int *hashes = new int[_numhashes];

    for (size_t p = 0; p < _numhashes; p++) {
        double s = 0;
        size_t i = 0;
        size_t j = 0;
        while (i < length & j < _samSize) {
            if (indices[i] == _indices[p][j]) {
                float v = values[i];
                s += v * _randBits[p][j];
                i++;
                j++;
            }
            else if (indices[i] < _indices[p][j]){
                i++;
            }
            else{
                j++;
            }
        }
        hashes[p] = (s >= 0 ? 0 : 1);
    }

    return hashes;
}


SparseRandomProjection::~SparseRandomProjection() {
    for (size_t i = 0; i < _numhashes; i++) {
        delete[]   _randBits[i];
        delete[]   _indices[i];
    }
    delete[]   _randBits;
    delete[]   _indices;
}
