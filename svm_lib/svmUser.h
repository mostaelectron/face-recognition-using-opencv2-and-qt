#ifndef SVMUSER_H
#define SVMUSER_H
#include <stdio.h>
#include <stdlib.h>
#include <String.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <QString>
#include "opencv2/opencv.hpp"
#include "svm_lib/svm.h"

    char* fromQStrToCstr(QString Qstr);
    void parse_command_line(int argc, char **argv, char *input_file_name, char *model_file_name);
    void read_problem(const char *filename);
    void do_cross_validation();
    void Train(int argc, char **argv);
    void print_null(const char *s);
    void exit_input_error(int line_num);
    void exit_with_help();
    static char* readline(FILE *input);
    QString imgToVect(cv::Mat& frame , int class_);
    void predict_image(char*Model, cv::Mat& img);
    int get_classe();
    float get_prob_estimates();



#endif // SVMUSER_H
