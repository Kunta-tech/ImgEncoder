#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QString>
#include <QPushButton>
#include <QSize>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <curl/curl.h>
#include <QByteArray>
#include <QImage>
#include <QDir>
#include <vector>
#include <opencv4/opencv2/opencv.hpp>

/**
* Author: Saugata Kundu
* Application Description: To Encode & Decode Images
*/
bool isImageLoaded;
cv::Mat cvImage_Global;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    isImageLoaded = false;
    ui->setupUi(this);
    connect(ui->LoadLocal,&QPushButton::clicked,this,&MainWindow::FunLoadLocal);
    connect(ui->LoadURL,&QPushButton::clicked,this,&MainWindow::FunLoadURL);
    connect(ui->Browse,&QPushButton::clicked,this,&MainWindow::Browse);
    connect(ui->Download,&QPushButton::clicked,this,&MainWindow::Download);
    ui->BeforeImg->setScaledContents(true);
    ui->AfterImg->setScaledContents(true);
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);
}
// Callback function for writing downloaded image data
size_t writeImageCallback(void* ptr, size_t size, size_t nmemb, std::vector<uchar>* data) {
    size_t dataSize = size * nmemb;
    std::copy((uchar*)ptr, (uchar*)ptr + dataSize, std::back_inserter(*data));
    return dataSize;
}
// Convert OpenCV Mat to QPixmap
QPixmap cvMatToQPixmap(const cv::Mat &cvImage) {
    QImage qImage(cvImage.data, cvImage.cols, cvImage.rows, cvImage.step, QImage::Format_RGB888);
    return QPixmap::fromImage(qImage.rgbSwapped());
}
double logisticMap(double x, double r){
    return r * x * (1 - x);
}
void MainWindow::encode(){

    isImageLoaded = true;

    QString PassPin = ui->PassPin->text();
    bool isNum;int p = PassPin.toInt(&isNum,10);
    if(PassPin.length()<4 or !isNum){
        QMessageBox::critical(nullptr,"Error", "The Pin is not proper\nPin: "+PassPin);
        return;
    }
    int pa = p/100,
           pb = p%100,
        tn = (int)((pb%10)/2) + 1;
    // Initializing the Chaotic Map
    double r = 4 - (pb/1e4);    // Control parameter
    double x = 0.3 + ((pa^pb)/1e5) + (pa/1e7);     // Initial value

//    The encryption algorithm
    for(int i=0;i<cvImage_Global.rows;i++){
        for(int j=0;j<cvImage_Global.cols;j++){
            for(int k=0;k<cvImage_Global.channels();k++){
                for(int t=0;t<tn;t++){
                    // Modify pixel values using the chaotic sequence (x)
                    x = logisticMap(x, r);
                    cvImage_Global.at<cv::Vec3b>(i, j)[k] ^= static_cast<uchar>(x * 255);
                }
            }
        }
    }// End of Algo

    // Convert cv::Mat to QImage
    ui->AfterImg->setPixmap(cvMatToQPixmap(cvImage_Global));
}

void MainWindow::FunLoadLocal(){
    QString txt = ui->Txt->toPlainText();
    if(txt == ""){
        QMessageBox::critical(nullptr, "Error", "Empty File Path!!");
        return;
    }
    // Load an image from file
    QFileInfo fileinfo = QFileInfo(txt);
    if(!fileinfo.exists() or !fileinfo.isFile()){
        QMessageBox::critical(nullptr,"Error", "Wrong File Path!!");
        return;
    }
    QPixmap pixmap(txt);

    // Check if the image was loaded successfully
    if (pixmap.isNull()) {
        QMessageBox::critical(nullptr,"Error", "File Can't be opened!!");
        return;
    }

    // Encode / Decode Image
    cvImage_Global = cv::imread(txt.toStdString());
    ui->BeforeImg->setPixmap(pixmap);
    encode();
}
void MainWindow::FunLoadURL(){
    std::string imageURL = ui->Txt->toPlainText().toStdString();
    if(imageURL == ""){
        QMessageBox::critical(nullptr, "Error", "Empty URL!!");
        return;
    }

    // Initialize cURL
    CURL* curl= curl_easy_init();
    if (curl) {
        // Set URL
        curl_easy_setopt(curl, CURLOPT_URL, imageURL.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeImageCallback);

        // Container to store downloaded image data
        std::vector<uchar> imageData;

        // Set curl option to write image data to the container
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &imageData);
        // Perform the request
        CURLcode result = curl_easy_perform(curl);

        // Check if request was successful (result == CURLE_OK)
        if (result == CURLE_OK) {
            QPixmap pixmap;

            // Decode the downloaded image data using OpenCV
            cvImage_Global = cv::imdecode(imageData, cv::IMREAD_COLOR); // Use appropriate flags

            // Display the image in QLabel
            pixmap = cvMatToQPixmap(cvImage_Global);
            ui->BeforeImg->setPixmap(pixmap);
            // Encode / Decode Image
            encode();
        } else {
            QMessageBox::critical(nullptr, "Error", "Failed to fetch the image");
            return;
        }

    } else {
        QMessageBox::critical(nullptr, "Error", "Failed to initialize cURL");
        return;
    }
    // Cleanup cURL
    curl_easy_cleanup(curl);


}
void MainWindow::Browse(){
    QString filepath = QFileDialog::getOpenFileName
        (nullptr, "Select a file", QDir::homePath(), "Image Files (*.png *.jpg *.jpeg)");
    ui->Txt->setPlainText(filepath);
}
void MainWindow::Download(){
    if (!isImageLoaded){
        QMessageBox::critical(nullptr, "Error", "No Image is loaded!!");
        return;
    }
    // Open a file dialog to select the save location
    QString savePath = QFileDialog::getSaveFileName(nullptr, "Save File", QDir::homePath(), "PNG File (*.png)");

    if (savePath.isEmpty()) return;
    
    // ui->AfterImg->pixmap()->save(savePath);
    imwrite(savePath.toStdString(), cvImage_Global);
    
}

MainWindow::~MainWindow()
{
    delete ui;
    curl_global_cleanup();
}

