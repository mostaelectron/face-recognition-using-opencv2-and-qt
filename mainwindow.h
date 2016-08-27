#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QTimer>
#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionOpen_triggered();

    void on_addTrain_triggered();

    void on_pushButton_clicked();

    void on_saveTrain_bt_clicked();

    void on_saveModel_bt_clicked();

    void on_addClass_bt_clicked();

    void on_actionStartRecognition_triggered();

    void on_actionStopRecognition_triggered();

    void on_actionStartTrain_triggered();

    void on_actionStopTrain_triggered();

    void on_actionOpenModel_triggered();

public slots:
    void detectFaceAndUpdateGui();
    void recognizeAndUpdateGui();

private:
    Ui::MainWindow *ui;
    void load_labels();
    QImage Mat2qimage(Mat& mat, QImage::Format format );
    Mat detectAndTrain(const Mat& image);
    Mat recongizeAndDraw(const Mat &image);
    CvSize getGuiImgSize();
    Mat convertToGray(Mat &image);
    bool openCaptureAndLoadCascadeFiles();
    bool closeCapture();
};

#endif // MAINWINDOW_H
