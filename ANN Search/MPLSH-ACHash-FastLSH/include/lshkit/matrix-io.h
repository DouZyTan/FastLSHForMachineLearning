/* 
    Copyright (C) 2008 Wei Dong <wdong@princeton.edu>. All Rights Reserved.
  
    This file is part of LSHKIT.
  
    LSHKIT is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LSHKIT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with LSHKIT.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * \file matrix-io.h
 * \brief Matrix I/O routines, inlined implementation (Matrix::load & Matrix::save).
 */

#ifndef LSHKIT_MATRIX_IO
#define LSHKIT_MATRIX_IO

#include<iostream>
#include <lshkit/common.h>
#include <cassert>
#include <fstream>

#ifdef MATRIX_MMAP
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#endif

using namespace std;

namespace lshkit{

template <typename T>
void Matrix<T>::peek (const std::string &path, int *elem_size, int *size, int *dim)
{
    unsigned header[3]; /* entry size, row, col */
    assert(sizeof header == 3*4);
    std::ifstream is(path.c_str(), std::ios::binary);
    BOOST_VERIFY(is);
    is.read((char *)header, sizeof header);
    BOOST_VERIFY(is);
    *elem_size = header[0];
    *size = header[1];
    *dim = header[2];
    /* entry size, row, col */

}


template <class T>
void Matrix<T>::load (std::istream &is)
{
    unsigned header[3]; /* entry size, row, col */
    assert(sizeof header == 3*4);
    is.read((char *)header, sizeof header);
    BOOST_VERIFY(is);
    BOOST_VERIFY(header[0] == sizeof(T));
    reset(header[2], header[1]);
    size_t sz = sizeof(T) * dim * N;
    is.read((char *)dims, sz);
    BOOST_VERIFY(is);
}


template <class T>
void Matrix<T>::loadFvecs (std::istream &is)
{
    
    uint32_t dim_;
    unsigned N_;
    is.read((char *) &dim_, sizeof(uint32_t));
    is.seekg(0,std::ios::end);
    std::ios::pos_type ss = is.tellg();
    size_t fsize = (size_t) ss;
    dim = dim_;
    N_ = (unsigned)(fsize / (dim + 1) / sizeof(uint32_t));
    reset(dim_, N_);
    is.clear();
	is.seekg(0, ios::beg);
    for (size_t i = 0; i < N; i++) {
        is.read((char *) &dim_, sizeof(uint32_t));
        if (dim_ != dim) {
            std::cout << "file error\n";
            exit(1);
        }
        is.read((char *) (dims + i * dim_), dim_ * sizeof(T));

    }
}

//data+query fvecs
template <class T>
void Matrix<T>::loadFvecsTwo (std::istream &is,std::istream &is2)
{
    
       //is
    uint32_t dim_;
    unsigned N_;
    is.read((char *) &dim_, sizeof(uint32_t));
    is.seekg(0,std::ios::end);
    std::ios::pos_type ss = is.tellg();
    size_t fsize = (size_t) ss;
    dim = dim_;
    N_ = (unsigned)(fsize / (dim + 1) / sizeof(uint32_t));
    printf("%d\n", N_);
    uint32_t dim2_;
    unsigned N2_;
    is2.read((char *) &dim2_, sizeof(uint32_t));
    is2.seekg(0,std::ios::end);
    std::ios::pos_type ss2 = is2.tellg();
    size_t fsize2 = (size_t) ss2;
    dim = dim2_;
    N2_ = (unsigned)(fsize2 / (dim + 1) / sizeof(uint32_t));
    reset(dim_, N_+N2_);
    is2.clear();
	is2.seekg(0, ios::beg);
    for (size_t i = 0; i < N2_; i++) {
        is2.read((char *) &dim2_, sizeof(uint32_t));
        if (dim2_ != dim) {
            std::cout << "file error\n";
            exit(1);
        }
        is2.read((char *) (dims + i * dim2_), dim2_ * sizeof(T));
    }

    is.clear();
	is.seekg(0, ios::beg);
    for (size_t i = N2_; i < N; i++) {
        is.read((char *) &dim_, sizeof(uint32_t));
        if (dim_ != dim) {
            std::cout << "file error\n";
            exit(1);
        }
        is.read((char *) (dims + i * dim_), dim_ * sizeof(T));
    }
  cout<<"data:"<<dims[ (N2_-1) * dim2_]<<std::endl;
 cout<<"data:"<<dims[ N2_* dim_]<<std::endl;
}


template <class T>
void Matrix<T>::save (std::ostream &os)
{
    unsigned header[3];
    header[0] = sizeof(T);
    header[1] = N;
    header[2] = dim;
    os.write((char *)header, sizeof header);
    BOOST_VERIFY(os);
    size_t sz = sizeof(T) * dim * N;
    os.write((char *)dims, sz);
    BOOST_VERIFY(os);

}

template <class T>
void Matrix<T>::load (const std::string &path)
{
    std::ifstream is(path.c_str(), std::ios::binary);
    load(is);
}

//new load-fvecs
template <class T>
void Matrix<T>::loadFvecs (const std::string &path)
{
    std::ifstream is(path.c_str(), std::ios::binary);
    loadFvecs(is);
}
//data+query fvecs
template <class T>
void Matrix<T>::loadFvecsTwo (const std::string &path,const std::string &path2)
{
    std::ifstream is(path.c_str(), std::ios::binary);
    std::ifstream is2(path2.c_str(), std::ios::binary);
    loadFvecsTwo(is,is2);
}

template <class T>
void Matrix<T>::save (const std::string &path)
{
    std::ofstream os(path.c_str(), std::ios::binary);
    save(os);
}

#ifdef MATRIX_MMAP
template <class T>
void Matrix<T>::map (const std::string &path) {

    if (dims != NULL) delete[] dims;

    int size = 0;

    fd = open(path.c_str(), O_RDONLY);
    BOOST_VERIFY(fd >= 0);

    if (read(fd, &size, 4) != sizeof(size)) BOOST_VERIFY(0);
    if (read(fd, &N, 4) != sizeof(N)) BOOST_VERIFY(0);
    if (read(fd, &dim, 4) != sizeof(dim)) BOOST_VERIFY(0);

    BOOST_VERIFY(size == sizeof(T));

    size_t sz = sizeof(T) * size_t(dim) * size_t(N);
    
    char *start = (char *)mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);
    if (start == MAP_FAILED) { BOOST_VERIFY(0); }
    dims = (T *)(start + 3 * 4);

}

template <class T>
void Matrix<T>::unmap () {
    char *start = (char *)dims - 3 * 4;
    if (munmap(start, sizeof(T) * size_t(dim) * size_t(N)) != 0) BOOST_VERIFY(0);
    close(fd);

    dims = 0;
    dim = 0;
    N = 0;
}

#endif

}

#endif

