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

#include "headers.h"
#include<ctime>
#include<random>


// The state vector for generation of random numbers.
char rngState[256];

// Initialize the random number generator.
void initRandom(){
  FAILIF(NULL == initstate(2, rngState, 256));
}

// Generate a random integer in the range [rangeStart,
// rangeEnd]. Inputs must satisfy: rangeStart <= rangeEnd.
IntT genRandomInt(IntT rangeStart, IntT rangeEnd){
  ASSERT(rangeStart <= rangeEnd);
  IntT r;
  r = rangeStart + (IntT)((rangeEnd - rangeStart + 1.0) * random() / (RAND_MAX + 1.0));
  ASSERT(r >= rangeStart && r <= rangeEnd);
  return r;
}

// Generate a random 32-bits unsigned (Uns32T) in the range
// [rangeStart, rangeEnd]. Inputs must satisfy: rangeStart <=
// rangeEnd.
Uns32T genRandomUns32(Uns32T rangeStart, Uns32T rangeEnd){
  ASSERT(rangeStart <= rangeEnd);
  Uns32T r;
  if (RAND_MAX >= rangeEnd - rangeStart) {
    r = rangeStart + (Uns32T)((rangeEnd - rangeStart + 1.0) * random() / (RAND_MAX + 1.0));
  } else {
    r = rangeStart + (Uns32T)((rangeEnd - rangeStart + 1.0) * ((LongUns64T)random() * ((LongUns64T)RAND_MAX + 1) + (LongUns64T)random()) / ((LongUns64T)RAND_MAX * ((LongUns64T)RAND_MAX + 1) + (LongUns64T)RAND_MAX + 1.0));
  }
  ASSERT(r >= rangeStart && r <= rangeEnd);
  return r;
}

// Generate a random real distributed uniformly in [rangeStart,
// rangeEnd]. Input must satisfy: rangeStart <= rangeEnd. The
// granularity of generated random reals is given by RAND_MAX.
RealT genUniformRandom(RealT rangeStart, RealT rangeEnd){
  ASSERT(rangeStart <= rangeEnd);
  RealT r;
  r = rangeStart + ((rangeEnd - rangeStart) * (RealT)random() / (RealT)RAND_MAX);
  ASSERT(r >= rangeStart && r <= rangeEnd);
  return r;
}

// Generate a random real from normal distribution N(0,1).
RealT FgenGaussianRandom(int dim){
  // Use Box-Muller transform to generate a point from normal
  // distribution.
  float delta = 0.01;
  float sigma = log(dim / delta) / dim;
  RealT x1, x2;
  do{
    x1 = genUniformRandom(0.0, sigma);
  } while (x1 == 0); // cannot take log of 0.
  x2 = genUniformRandom(0.0, sigma);
  RealT z;
  z = SQRT(-2.0 * LOG(x1)) * COS(2.0 * M_PI * x2);
  return z;
}

RealT FFgenGaussianRandom(int dim){
  // Use Box-Muller transform to generate a point from normal
  // distribution.
  float delta = 0.01;
  float sigma = log(dim / delta) / dim;
  float mean = 0;
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::normal_distribution<float> dist(mean, sigma);
  return dist(gen);
}

void CreateDiagonal(int dim, int *Diagonal)
{
    // srand(time(NULL));
    for(size_t i = 0; i < dim; ++i){
        float probability = rand() % (NUM + 1) / (float)(NUM + 1);
        if (probability > 0.5){
            Diagonal[i] = 1;
        }else{
           Diagonal[i] = -1; 
        }
    }
}

RealT genGaussianRandom(){
  // Use Box-Muller transform to generate a point from normal
  // distribution.
  // float delta = 0.1;
  // float sigma = log(dim / delta) / dim;
  // srand((unsigned)(time(0)));
  RealT x1, x2;
  do{
    x1 = genUniformRandom(0.0, 1.0);
  } while (x1 == 0); // cannot take log of 0.
  x2 = genUniformRandom(0.0, 1.0);
  RealT z;
  z = SQRT(-2.0 * LOG(x1)) * COS(2.0 * M_PI * x2);
  return z;
}

// Generate a random real from Cauchy distribution N(0,1).
RealT genCauchyRandom(){
  RealT x, y;
  x = genGaussianRandom();
  y = genGaussianRandom();
  if (ABS(y) < 0.0000001) {
    y = 0.0000001;
  }
  return x / y;
}
