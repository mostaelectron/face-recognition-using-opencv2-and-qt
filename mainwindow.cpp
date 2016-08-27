#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>

#include "svm_lib/svmUser.h"//svm lib
#include "preprocessFace.h"     // preprocess face images



/** Global variables */
const String face_cascade_name =  "haarcascade/haarcascade_frontalface_alt.xml";
const String eyes_cascade_name =  "haarcascade/haarcascade_eye.xml";
const String eyes_cascade_name2 = "haarcascade/haarcascade_eye_tree_eyeglasses.xml";
const String nose_cascade_name =  "haarcascade/haarcascade_mcs_nose.xml";
CascadeClassifier face_cascade;
CascadeClassifier eyes_cascade;
CascadeClassifier eyes_cascade2;
CascadeClassifier nose_cascade;

VideoCapture capture_cam;
QTimer *trainingTimer;
QTimer *recognitionTimer;
const String window_name = "Capture - Face detection";

const CvSize StandardSize = cvSize(500,500);
const CvSize TrainSize = cvSize( 30 , 30);


//add train flag
bool addTrain_f = false;
//labels txt filename
const QString labelsFileName = "labels/labels.txt";


//string array for train
char** name_parametre;
QString modelFilename;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


     ///load stored labels from txt file
     load_labels();
     ///initialize svm params vector
     name_parametre=new char* [sizeof(char*)*7];
     name_parametre[0]=new char [sizeof(char)*100];//fonction
     name_parametre[1]=new char [sizeof(char)*100];//-t
     name_parametre[2]=new char [sizeof(char)*100];//0
     name_parametre[3]=new char [sizeof(char)*100];//-c
     name_parametre[4]=new char [sizeof(char)*100];//100
     name_parametre[5]=new char [sizeof(char)*100];//traion.trn
     name_parametre[6]=new char [sizeof(char)*100]; // model.mdl

     trainingTimer = new QTimer(this);
     connect(trainingTimer, SIGNAL(timeout()), this, SLOT(detectFaceAndUpdateGui()));

     recognitionTimer = new QTimer(this);
     connect(recognitionTimer, SIGNAL(timeout()), this, SLOT(recognizeAndUpdateGui()));

}

/// Load stored labels from txt file
void MainWindow::load_labels()
{
    QFile file(labelsFileName);
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(0, "Loadding labels file Error", file.errorString());
    }
    QTextStream in(&file);
    while(!in.atEnd()) {
        QString label = in.readLine();
        ui->classes_cBox->addItem(label);
    }
    file.close();
}
///////////////////////////////////////////////////

MainWindow::~MainWindow()
{
    delete ui;
}

///open train file
void MainWindow::on_actionOpen_triggered()
{
}
/////////////////////////////////////////////

///Open capture cam and load cascades files
bool MainWindow::openCaptureAndLoadCascadeFiles()
{
    //if fail whene loading cascade xml files
    if( !face_cascade.load( face_cascade_name ) || !eyes_cascade.load( eyes_cascade_name ) || !eyes_cascade2.load( eyes_cascade_name2 ) || !nose_cascade.load( nose_cascade_name ))
        return false;

    capture_cam.open(-1);
    //if fail whene openning cam
    if(! capture_cam.isOpened())
        return false;
    return true;
}
/////////////////////////////////////////////

///close capture cam
bool MainWindow::closeCapture()
{
    if(capture_cam.isOpened())
        capture_cam.release();
    return capture_cam.isOpened();
}
/////////////////////////////////////////////


///Start trainig
void MainWindow::on_actionStartTrain_triggered()
{
    openCaptureAndLoadCascadeFiles();
    trainingTimer->start(20);
    ui->actionStartTrain->setDisabled(true);
    ui->actionStopTrain->setDisabled(false);
    ui->actionStartRecognition->setDisabled(true);
}
/////////////////////////////////////////////////////////////////////////////

///Stop training
void MainWindow::on_actionStopTrain_triggered()
{
    trainingTimer->stop();
    closeCapture(); //close capture cam
    ui->actionStartTrain->setDisabled(false);
    ui->actionStopTrain->setDisabled(true);
    ui->actionStartRecognition->setDisabled(false);
}
/////////////////////////////////////////////////////////////////////////////

///return GUI image size
CvSize MainWindow::getGuiImgSize()
{
    return cvSize( ui->des_img->width() , ui->des_img->height());
}
/////////////////////////////////////////////////////////////////////////////


///conevrt image t gray scal
Mat MainWindow::convertToGray(Mat & image)
{
    Mat gray_img ;
    //convert to gray scal
    if (image.channels() == 3) {
        cvtColor(image, gray_img, CV_BGR2GRAY);
    }else if (image.channels() == 4) {
        cvtColor(image, gray_img, CV_BGRA2GRAY);
    }else { //if gray
        gray_img = image; // get just refrence
    }
    return gray_img;
}
///////////////////// ProcessFrameAndUpdateGui //////////////////////////////////////
void MainWindow::detectFaceAndUpdateGui()
{
    Mat src_img, des_img;
    capture_cam.read(src_img);
    if(src_img.empty())
        return;
    //resize raw image
    cv::resize( src_img, src_img, StandardSize );
    //detect face for t
    des_img = detectAndTrain(src_img);

    //show results
    cv::resize( des_img, des_img, getGuiImgSize() );
    ui->des_img->setPixmap(QPixmap::fromImage(Mat2qimage(des_img, QImage::Format_RGB888)));
}
/////////////////////////////////////////////////////////////////////////////////////////////

//detect faces and draw theme
Mat MainWindow::detectAndTrain(const Mat& image)
{
    Mat copy_img ; // copy of original image
    Mat gray_img ; // gray scal image
    Mat detectedFace; // used for detect face
    Mat processedFace; //used for process face

    copy_img = image.clone();
    gray_img = convertToGray(copy_img); // convert to gray
    equalizeHist( gray_img, gray_img ); //equalize to incease edges

    //-- Detect face
    vector<Rect> rectFaces;
    face_cascade.detectMultiScale( gray_img, rectFaces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(20, 20) );
    ///if face detected
    if(rectFaces.size()>0)
    {
        //get only the first one
        detectedFace = gray_img(rectFaces[0]);
        //processedFace = processDetectedFace(detectedFace , 30 , eyes_cascade , eyes_cascade2 , true); //process detected face
        processedFace = processDetectedFace_(detectedFace , 30 , nose_cascade ); //process detected face
        if(processedFace.rows > 0 )
        {
            rectangle(copy_img, rectFaces[0] , Scalar( 255, 0, 0 ), 2); //draw rect on detected face area

            //Draw processed face on top left of BRG copy_img
            for(int i=0; i<processedFace.rows; ++i)
            {
                for(int j=0; j<processedFace.cols; ++j)
                {
                    copy_img.at<cv::Vec3b>(j,i)[0] = processedFace.at<uchar>(j,i);
                    copy_img.at<cv::Vec3b>(j,i)[1] = processedFace.at<uchar>(j,i);
                    copy_img.at<cv::Vec3b>(j,i)[2] = processedFace.at<uchar>(j,i);
                }
            }
            ///Train this face using processedFace image (size = imgTrainSize)
            cv::resize(processedFace, processedFace, TrainSize);
            if(addTrain_f)
            {
                addTrain_f = false;
                int classNum = ui->classes_cBox->currentIndex() + 1 ; //index start from 2 (index 1 is reserved for recognition)
                QString str = QString(imgToVect(processedFace, classNum));
                ui->train_text->setPlainText( ui->train_text->toPlainText() + str + "\n");
            }

        }
    }
    ///End of for each face
    return copy_img;     /// return result image
}
/////////////////////////////////////////////////////////////////////////////


///convert from Mat to QImage using QImage::Format
QImage MainWindow::Mat2qimage(Mat& mat, QImage::Format format )
{
    //if not gray image
    if(format!=QImage::Format_Indexed8)
        cvtColor(mat, mat, CV_BGR2RGB); // if color image  , convert from BRG to RGB
    return QImage((uchar*)mat.data , mat.cols, mat.rows, mat.step ,  format);
}
/////////////////////////////////////////////////////////////////////////////



///add vect to train
void MainWindow::on_addTrain_triggered()
{
    addTrain_f = true;
}
/////////////////////////////////////////////////////////////////////////////


void MainWindow::on_pushButton_clicked()
{

}

//save Train file
void MainWindow::on_saveTrain_bt_clicked()
{
    QString trainContent = ui->train_text->toPlainText();
    if(! trainContent.isEmpty() )
    {
          QString filename = QFileDialog::getSaveFileName(this, tr("Save File"), "train", tr("Train File (*.trn)"));
          filename = QDir::fromNativeSeparators(filename);
          QFile file(filename);
            if (file.open(QIODevice::ReadWrite|QIODevice::Truncate))
            {
                QTextStream stream(&file);
                stream << trainContent;
                file.close();
                name_parametre[5] = fromQStrToCstr(filename);
                qDebug() << QString(name_parametre[5]);
                qDebug() << "file written!!";
            }
            else
            {
                qDebug() << "Error when writing file!!";
            }
    }
    else
    {
    }
}
/////////////////////////////////////////////////////////////////////////////

///save model file
void MainWindow::on_saveModel_bt_clicked()
{
    //foction -t 0 -c 100 trin.trn model.mdl
    //   0     1 2  3  4      5        6
    name_parametre[1]="-t";
    name_parametre[2]="3";
    name_parametre[3]="-c";
    name_parametre[4]="1000000";
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Model"), "model", tr("Model File (*.mdl)"));
    filename = QDir::fromNativeSeparators(filename);
    modelFilename = filename;
    name_parametre[6] = fromQStrToCstr(filename);
    qDebug() << QString(name_parametre[6]);
    Train(7, name_parametre);
}
/////////////////////////////////////////////////////////////////////////////

///add label class
void MainWindow::on_addClass_bt_clicked()
{
    QString label = ui->class_editText->text();
    if( ! label.isEmpty() )
    {
        ui->classes_cBox->addItem(label);
        ///append label to labels txt file
        QFile file(labelsFileName);
        if (file.open(QIODevice::Append))
        {
            QTextStream stream(&file);
            stream << label << "\n";
            file.close();
            qDebug() << "new label added!";
        }
        else
        {
            qDebug() << "Error when adding new label";
        }
        ////////////////////////////////////
    }
}
/////////////////////////////////////////////



///Start recognition
void MainWindow::on_actionStartRecognition_triggered()
{
    if(! modelFilename.isEmpty())
    {
        openCaptureAndLoadCascadeFiles();
        recognitionTimer->start(20);
        ui->actionStartRecognition->setDisabled(true);
        ui->actionStopRecognition->setDisabled(false);
        ui->actionStartTrain->setDisabled(true);
    }
    else
    {
        qDebug() << "empty model file!!";
    }
}
/////////////////////////////////////////////////////////////////////////////

///Stop recognition
void MainWindow::on_actionStopRecognition_triggered()
{
    recognitionTimer->stop();
    closeCapture(); //close capture cam
    ui->actionStartRecognition->setDisabled(false);
    ui->actionStopRecognition->setDisabled(true);
    ui->actionStartTrain->setDisabled(false);
}
/////////////////////////////////////////////////////////////////////////////

///////////////////// RecognizeAndUpdateGui //////////////////////////////////////
void MainWindow::recognizeAndUpdateGui()
{
    Mat src_img, des_img;
    capture_cam.read(src_img);
    if(src_img.empty())
        return;
    cv::resize(src_img, src_img, getGuiImgSize());
    des_img = recongizeAndDraw(src_img);
    //show results
    ui->des_img->setPixmap(QPixmap::fromImage(Mat2qimage(des_img, QImage::Format_RGB888)));
}
///////////////////////////////////////////////////////////////////////////////////////////////


//recongize faces and draw theme
Mat MainWindow::recongizeAndDraw(const Mat& image)
{
    Mat copy_img ; // copy of original image
    Mat gray_img ; // gray scal image
    Mat detectedFace; // used for detect face
    Mat processedFace; //used for process face

    copy_img = image.clone();
    gray_img = convertToGray(copy_img); // convert to gray
    equalizeHist( gray_img, gray_img ); //equalize to incease edges
    //-- Detect faces
    vector<Rect> rectFaces;
    face_cascade.detectMultiScale( gray_img, rectFaces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(20, 20) );
    ///for each faces
    for( int i = 0; i < rectFaces.size(); i++ )
    {
        detectedFace = gray_img(rectFaces[i]);
        processedFace = processDetectedFace(detectedFace , 30 , eyes_cascade , eyes_cascade2 , true);
        if(processedFace.rows > 0 )
        {
            predict_image(fromQStrToCstr(modelFilename) ,processedFace); //predict face
            int classe = get_classe();
            QString label = ui->classes_cBox->itemText( get_classe() - 1);
            Point orgText = Point(rectFaces[i].x , rectFaces[i].y);
            putText(copy_img , label.toStdString() , orgText , FONT_ITALIC , 1 , Scalar( 255, 0, 0 ));
            qDebug() << "label predicted:" << label;
        }
    }
    ///End of for each face
    return copy_img;     /// return result image
}
/////////////////////////////////////////////////////////////////////////////

///open model file
void MainWindow::on_actionOpenModel_triggered()
{
    QFileDialog fileDialog;
    QString filename = QFileDialog::getOpenFileName(this, tr("Open Model file"), "", tr("Model File (*.mdl)"));
    filename = QDir::fromNativeSeparators(filename);
    modelFilename = filename;
    qDebug()<<modelFilename;
}
/////////////////////////////////////////////////////////////////////////////
