#include "SignedRandomProjection.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
using namespace std;

/* Author: Chen Luo and Anshumali Shrivastava
*  COPYRIGHT PROTECTION
*  Free for research use.
*  For commercial use, contact:  RICE UNIVERSITY INVENTION & PATENT or the Author.
*/

SignedRandomProjection::SignedRandomProjection(int dimention, int numOfHashes) {
    _dim = dimention;
    _numhashes = numOfHashes;
    // _samSize = ceil(0.25 * _dim);// for ACHash 
    _samSize = 30;   // for FastLSH

    int *a = new int[_dim];
    for (size_t i = 0; i < _dim; i++) {
        a[i] = i;
    }


    srand(time(0));
    std::random_device rd;
    std::mt19937 e2(rd());

    

    //*******generate hash function for ACE
    std::normal_distribution<> dist(0, 1);

    _randBits = new double*[_numhashes];
    for (size_t i = 0; i < _numhashes; i++) {
        _randBits[i] = new double[_dim];
        for (size_t j = 0; j < _dim; j++) {
            _randBits[i][j]=dist(e2);
        }
    }

    //*******generate hash function for FastACE

    // _randBits = new double*[_numhashes];
    // _indices = new int *[_numhashes];
    // for (size_t i = 0; i < _numhashes; i++) {
    //     random_shuffle(&a[0], &a[_dim - 1]);
    //     _randBits[i] = new double[_samSize];
    //     _indices[i] = new int[_samSize];
    //     for (size_t j = 0; j < _samSize; j++) {
    //         _indices[i][j] = a[j];
    //         _randBits[i][j]=dist(e2);
    //     }
    // }

    //*******generate hash function for ACHash
    // double esp = 0.01;
    // double sigma = log2(_dim / esp) / _dim;
    // std::normal_distribution<> dist(0, sigma);

    // _randBits = new double*[_numhashes];
    // _indices = new int *[_numhashes];
    // for (size_t i = 0; i < _numhashes; i++) {
    //     random_shuffle(&a[0], &a[_dim - 1]);
    //     _randBits[i] = new double[_samSize];
    //     _indices[i] = new int[_samSize];
    //     for (size_t j = 0; j < _samSize; j++) {
    //         _indices[i][j] = a[j];
    //         _randBits[i][j]=dist(e2);
    //     }
    // }
}

int *SignedRandomProjection::getHash(double *vector, int length) {
    int *hashes = new int[_numhashes];
#pragma omp parallel for
    for (int i = 0; i < _numhashes; i++) {
        double s = 0;

        for (size_t j = 0; j < _samSize; j++) {
            double v = vector[_indices[i][j]];
            s+=v*_randBits[i][j];
        }

        hashes[i] = (s >= 0 ? 1 : 0);
    }
    return hashes;
}

SignedRandomProjection::~SignedRandomProjection() {
    for (size_t i = 0; i < _numhashes; i++) {
        delete[]   _randBits[i];
        delete[]   _indices[i];
    }
    delete[]   _randBits;
    delete[]   _indices;
}
