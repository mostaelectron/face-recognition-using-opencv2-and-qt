#include "svmUser.h"


using namespace std;


#define Malloc(type,n) (type *)malloc((n)*sizeof(type))

struct svm_node *x;
int max_nr_attr = 64;
struct svm_model* model2;
int predict_probability=0;
float estimatedProb;
string line2 ="";
static int resultat_classe;



struct svm_parameter param;		// set by parse_command_line
struct svm_problem prob;		// set by read_problem
struct svm_model *model;
struct svm_node *x_space;
int cross_validation;
int nr_fold;

static char *line = NULL;
static int max_line_len;


///convert Qstring to c string (char*)
char* fromQStrToCstr(QString Qstr)
{
    char* cstr;
    string sdtStr = Qstr.toStdString();
    cstr = new char [sdtStr.size()+1];
    strcpy( cstr, sdtStr.c_str() );
    return cstr;
}
////////////////////////////////////////////////////////////////

void print_null(const char *s) {}

void exit_input_error(int line_num)
{
    fprintf(stderr,"Wrong input format at line %d\n", line_num);
    exit(1);
}

void exit_with_help()
{
    printf(
    "Usage: svm-train [options] training_set_file [model_file]\n"
    "options:\n"
    "-s svm_type : set type of SVM (default 0)\n"
    "	0 -- C-SVC\n"
    "	1 -- nu-SVC\n"
    "	2 -- one-class SVM\n"
    "	3 -- epsilon-SVR\n"
    "	4 -- nu-SVR\n"
    "-t kernel_type : set type of kernel function (default 2)\n"
    "	0 -- linear: u'*v\n"
    "	1 -- polynomial: (gamma*u'*v + coef0)^degree\n"
    "	2 -- radial basis function: exp(-gamma*|u-v|^2)\n"
    "	3 -- sigmoid: tanh(gamma*u'*v + coef0)\n"
    "	4 -- precomputed kernel (kernel values in training_set_file)\n"
    "-d degree : set degree in kernel function (default 3)\n"
    "-g gamma : set gamma in kernel function (default 1/k)\n"
    "-r coef0 : set coef0 in kernel function (default 0)\n"
    "-c cost : set the parameter C of C-SVC, epsilon-SVR, and nu-SVR (default 1)\n"
    "-n nu : set the parameter nu of nu-SVC, one-class SVM, and nu-SVR (default 0.5)\n"
    "-p epsilon : set the epsilon in loss function of epsilon-SVR (default 0.1)\n"
    "-m cachesize : set cache memory size in MB (default 100)\n"
    "-e epsilon : set tolerance of termination criterion (default 0.001)\n"
    "-h shrinking : whether to use the shrinking heuristics, 0 or 1 (default 1)\n"
    "-b probability_estimates : whether to train a SVC or SVR model for probability estimates, 0 or 1 (default 0)\n"
    "-wi weight : set the parameter C of class i to weight*C, for C-SVC (default 1)\n"
    "-v n: n-fold cross validation mode\n"
    "-q : quiet mode (no outputs)\n"
    );
    exit(1);
}

static char* readline(FILE *input)
{
    int len;

    if(fgets(line,max_line_len,input) == NULL)
        return NULL;

    while(strrchr(line,'\n') == NULL)
    {
        max_line_len *= 2;
        line = (char *) realloc(line,max_line_len);
        len = (int) strlen(line);
        if(fgets(line+len,max_line_len-len,input) == NULL)
            break;
    }
    return line;
}

void Train(int argc, char **argv)
{
    char input_file_name[1024];
    char model_file_name[1024];
    const char *error_msg;

    parse_command_line(argc, argv, input_file_name, model_file_name);

    read_problem(input_file_name);

    error_msg = svm_check_parameter(&prob,&param);
    if(error_msg)
    {
        fprintf(stderr,"Error: %s\n",error_msg);
        exit(1);
    }

    if(cross_validation)
    {
        do_cross_validation();
    }
    else
    {
        model = svm_train(&prob,&param);
        svm_save_model(model_file_name,model);
        svm_destroy_model(model);
    }

    svm_destroy_param(&param);
/*
    free(prob.y);
    free(prob.x);
    free(x_space);
    free(line);
*/
}

void do_cross_validation()
{
    int i;
    int total_correct = 0;
    double total_error = 0;
    double sumv = 0, sumy = 0, sumvv = 0, sumyy = 0, sumvy = 0;
    double *target = Malloc(double,prob.l);

    svm_cross_validation(&prob,&param,nr_fold,target);
    if(param.svm_type == EPSILON_SVR ||
       param.svm_type == NU_SVR)
    {
        for(i=0;i<prob.l;i++)
        {
            double y = prob.y[i];
            double v = target[i];
            total_error += (v-y)*(v-y);
            sumv += v;
            sumy += y;
            sumvv += v*v;
            sumyy += y*y;
            sumvy += v*y;
        }
        printf("Cross Validation Mean squared error = %g\n",total_error/prob.l);
        printf("Cross Validation Squared correlation coefficient = %g\n",
            ((prob.l*sumvy-sumv*sumy)*(prob.l*sumvy-sumv*sumy))/
            ((prob.l*sumvv-sumv*sumv)*(prob.l*sumyy-sumy*sumy))
            );
    }
    else
    {
        for(i=0;i<prob.l;i++)
            if(target[i] == prob.y[i])
                ++total_correct;
        printf("Cross Validation Accuracy = %g%%\n",100.0*total_correct/prob.l);
    }
    free(target);
}

void parse_command_line(int argc, char **argv, char *input_file_name, char *model_file_name)
{
    int i;

    // default values
    param.svm_type = C_SVC;
    param.kernel_type = RBF;
    param.degree = 3;
    param.gamma = 0;	// 1/k
    param.coef0 = 0;
    param.nu = 0.5;
    param.cache_size = 100;
    param.C = 1;
    param.eps = 1e-3;
    param.p = 0.1;
    param.shrinking = 1;
    param.probability = 0;
    param.nr_weight = 0;
    param.weight_label = NULL;
    param.weight = NULL;
    cross_validation = 0;

    // parse options
    for(i=1;i<argc;i++)
    {
        if(argv[i][0] != '-') break;
        if(++i>=argc)
            exit_with_help();
        switch(argv[i-1][1])
        {
            case 's':
                param.svm_type = atoi(argv[i]);
                break;
            case 't':
                param.kernel_type = atoi(argv[i]);
                break;
            case 'd':
                param.degree = atoi(argv[i]);
                break;
            case 'g':
                param.gamma = atof(argv[i]);
                break;
            case 'r':
                param.coef0 = atof(argv[i]);
                break;
            case 'n':
                param.nu = atof(argv[i]);
                break;
            case 'm':
                param.cache_size = atof(argv[i]);
                break;
            case 'c':
                param.C = atof(argv[i]);
                break;
            case 'e':
                param.eps = atof(argv[i]);
                break;
            case 'p':
                param.p = atof(argv[i]);
                break;
            case 'h':
                param.shrinking = atoi(argv[i]);
                break;
            case 'b':
                param.probability = atoi(argv[i]);
                break;
            case 'q':
                svm_print_string = &print_null;
                i--;
                break;
            case 'v':
                cross_validation = 1;
                nr_fold = atoi(argv[i]);
                if(nr_fold < 2)
                {
                    fprintf(stderr,"n-fold cross validation: n must >= 2\n");
                    exit_with_help();
                }
                break;
            case 'w':
                ++param.nr_weight;
                param.weight_label = (int *)realloc(param.weight_label,sizeof(int)*param.nr_weight);
                param.weight = (double *)realloc(param.weight,sizeof(double)*param.nr_weight);
                param.weight_label[param.nr_weight-1] = atoi(&argv[i-1][2]);
                param.weight[param.nr_weight-1] = atof(argv[i]);
                break;
            default:
                fprintf(stderr,"Unknown option: -%c\n", argv[i-1][1]);
                exit_with_help();
        }
    }

    // determine filenames

    if(i>=argc)
        exit_with_help();

    strcpy(input_file_name, argv[i]);

    if(i<argc-1)
        strcpy(model_file_name,argv[i+1]);
    else
    {
        char *p = strrchr(argv[i],'/');
        if(p==NULL)
            p = argv[i];
        else
            ++p;
        sprintf(model_file_name,"%s.model",p);
    }
}

// read in a problem (in svmlight format)

void read_problem(const char *filename)
{
    int elements, max_index, inst_max_index, i, j;
    FILE *fp = fopen(filename,"r");
    char *endptr;
    char *idx, *val, *label;

    if(fp == NULL)
    {
        fprintf(stderr,"can't open input file %s\n",filename);
        exit(1);
    }

    prob.l = 0;
    elements = 0;

    max_line_len = 1024;
    line = Malloc(char,max_line_len);
    while(readline(fp)!=NULL)
    {
        char *p = strtok(line," \t"); // label

        // features
        while(1)
        {
            p = strtok(NULL," \t");
            if(p == NULL || *p == '\n') // check '\n' as ' ' may be after the last feature
                break;
            ++elements;
        }
        ++elements;
        ++prob.l;
    }
    rewind(fp);

    prob.y = Malloc(double,prob.l);
    prob.x = Malloc(struct svm_node *,prob.l);
    x_space = Malloc(struct svm_node,elements);

    max_index = 0;
    j=0;
    for(i=0;i<prob.l;i++)
    {
        inst_max_index = -1; // strtol gives 0 if wrong format, and precomputed kernel has <index> start from 0
        readline(fp);
        prob.x[i] = &x_space[j];
        label = strtok(line," \t");
        prob.y[i] = strtod(label,&endptr);
        if(endptr == label)
            exit_input_error(i+1);

        while(1)
        {
            idx = strtok(NULL,":");
            val = strtok(NULL," \t");

            if(val == NULL)
                break;

            errno = 0;
            x_space[j].index = (int) strtol(idx,&endptr,10);
            if(endptr == idx || errno != 0 || *endptr != '\0' || x_space[j].index <= inst_max_index)
                exit_input_error(i+1);
            else
                inst_max_index = x_space[j].index;

            errno = 0;
            x_space[j].value = strtod(val,&endptr);
            if(endptr == val || errno != 0 || (*endptr != '\0' && !isspace(*endptr)))
                exit_input_error(i+1);

            ++j;
        }

        if(inst_max_index > max_index)
            max_index = inst_max_index;
        x_space[j++].index = -1;
    }

    if(param.gamma == 0 && max_index > 0)
        param.gamma = 1.0/max_index;

    if(param.kernel_type == PRECOMPUTED)
        for(i=0;i<prob.l;i++)
        {
            if (prob.x[i][0].index != 0)
            {
                fprintf(stderr,"Wrong input format: first column must be 0:sample_serial_number\n");
                exit(1);
            }
            if ((int)prob.x[i][0].value <= 0 || (int)prob.x[i][0].value > max_index)
            {
                    fprintf(stderr,"Wrong input format: sample_serial_number out of range\n");
                 exit(1);
            }
        }

    fclose(fp);
}


///////////////////////Convert image to svm vect/////////////////////////////////////
QString imgToVect(cv::Mat& frame , int class_)
{
QString vect_str;
double val;

    vect_str = QString::number(class_) + " ";
    int ind = 1;

     for(int i=0; i<frame.rows ; i++)
      for(int j=0; j<frame.cols ; j++)
      {
            val = frame.at<uchar>(i,j)/256.0;
            if( val!=0 )
             {
                vect_str+= QString::number(ind) + ":" + QString::number(val,'g',4)+ " ";
                ind++;
             }
       }
    return vect_str;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------

///predict image
void predict_image(char*Model, cv::Mat& img)
{
    double *prob_estimates = NULL;

    model2=svm_load_model(Model);
    x = (struct svm_node *) malloc((max_nr_attr)*sizeof(struct svm_node));
    int correct = 0;
    int total = 0;
    double error = 0;
    double sump = 0, sumt = 0, sumpp = 0, sumtt = 0, sumpt = 0;
    int svm_type=svm_get_svm_type(model2);

//---------------------------------------------------------------------

     int i = 0;

     double target_label, predict_label;
     char *idx, *val, *label, *endptr;
     int inst_max_index = -1; // strtol gives 0 if wrong format, and precomputed kernel has <index> start from 0
     label = strtok(fromQStrToCstr(imgToVect(img , 1 )) , " \t");
     target_label = strtod(label,&endptr);
          while(1)
        {
            if(i>=max_nr_attr-1)	// need one more for index = -1
            {
                    max_nr_attr *= 2;
                    x = (struct svm_node *) realloc(x,max_nr_attr*sizeof(struct svm_node));
            }

            idx = strtok(NULL,":");
            val = strtok(NULL," \t");

            if(val == NULL)
                break;
            errno = 0;
            x[i].index = (int) strtol(idx,&endptr,10);
                inst_max_index = x[i].index;
                        errno = 0;
                x[i].value = strtod(val,&endptr);
                ++i;
        }
        x[i].index = -1;

        if (predict_probability && (svm_type==C_SVC || svm_type==NU_SVC))
        {
            predict_label = svm_predict_probability(model2,x,prob_estimates);
        }
        else
        {
            predict_label = svm_predict(model2,x);
            resultat_classe=int(predict_label);
        }

        if(predict_label == target_label)
            ++correct;
        error += (predict_label-target_label)*(predict_label-target_label);
        sump += predict_label;
        sumt += target_label;
        sumpp += predict_label*predict_label;
        sumtt += target_label*target_label;
        sumpt += predict_label*target_label;
        ++total;

        if(predict_probability)
        {
            estimatedProb = *prob_estimates;
            free(prob_estimates);
        }
        svm_destroy_model(model2);
        free(x);
}

///return predicted class
int get_classe()
{
    return resultat_classe;
}
/////////////////////////////////////////////////////////////////////

///return estimated probability
float get_prob_estimates()
{
    return estimatedProb;
}

/////////////////////////////////////////////////////////////////////
