#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <cstring>
#include <chrono>
#include <cmath>
#include "SignedRandomProjection.h"
#include "LSH_Ourlier.h"

using namespace std;
double precision;
double recall;
void process(string name, bool *outlier, int numData, int num_real_outliers, double time, int K, int L, double alpha) {
    int all = 0, correct = 0, union_with_truth = 0;
    for (int i = 0; i < numData; i++) {
        bool real_outlier = i < num_real_outliers;
        if (outlier[i] || real_outlier) {
            union_with_truth++;
        }
        if (outlier[i]) {
            all++;
            if (real_outlier) {
                correct++;
            }
        }
    }
    double overlap = (double) correct;
    precision = overlap / all;
    recall = overlap / num_real_outliers;
    double jaccard = overlap / union_with_truth;
    printf("%s,%d,%d,%f,%f,%d,%d,%f,%f,%f\n", name.c_str(), K, L, alpha, time, all, correct, precision, recall,
           jaccard);
}

int main(int argc, char *argv[]) {
    //Data
    int dim = 10, num_real_outliers = 879;
    string filename = "shuttle.csv";
    // int dim = 124, num_real_outliers = 7841;
    // string filename = "a9a.csv";
    // int dim = 167, num_real_outliers = 97;
    // string filename = "musk2.csv";
    std::vector<double *> data = load(filename, dim);
    int numData = (int) data.size();

    printf("Input data: filename = %s, # data points = %d, dimension = %d\n", filename.c_str(), numData, dim);
    printf("Gold standard total outliers = %d\n", num_real_outliers);
    printf("Name,K,L,Alpha,Time (seconds),Total reported outliers,Overlap with gold standard,Precision,Recall,Jaccard similarity with gold standard\n");
    double alpha = 2.15;
    int num_iterations = 10;
    double total_precision = 0.0;
    double total_recall = 0.0;
    double total_time = 0.0;
    for (int K = 15; K < 16; K += 5) {
        for (int L = 50; L < 51; L += 10) {
            // for (double alpha = 0; alpha <= 3; alpha += 0.05) {
                for (double i = 0; i < num_iterations; i++) {
                double time;
                bool *lsh_result = lsh(data, numData, dim, &time, K, L, alpha);
                process("LSH", lsh_result, numData, num_real_outliers, time, K, L, alpha);
                total_precision += precision;
                total_recall += recall;
                total_time += time;
                delete[] lsh_result;
            }
            printf("%f,%f,%f\n", total_precision/num_iterations, total_recall/num_iterations, total_time/num_iterations);
            total_precision = 0;
            total_recall = 0;
            total_time = 0;
        }
    }
}
