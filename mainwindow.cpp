// Yet anther C++ implementation of EVM, based on OpenCV and Qt. 
// Copyright (C) 2014  Joseph Pan <cs.wzpan@gmail.com>
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 USA
// 


#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    rateLabel = new QLabel;         // Frame rate
    posLabel = new QLabel;          // mouse position

    this->centralWidget()->setMouseTracking(true);
    this->setMouseTracking(true); // allow tracking mouse

    // remind users to open a video
    inputTip = "Please open a video.";
    ui->videoLabel->setText(inputTip);

    // Status bar
    rateLabel->setText("");
    ui->statusBar->addPermanentWidget(rateLabel);
    posLabel->setText("");;
    ui->statusBar->addPermanentWidget(posLabel);

    // progress dialog
    progressDialog =  0;

    updateStatus(false);

    video = new VideoProcessor;

    connect(video, SIGNAL(showFrame(cv::Mat)), this, SLOT(showFrame(cv::Mat)));
    connect(video, SIGNAL(sleep(int)), this, SLOT(sleep(int)));
    connect(video, SIGNAL(revert()), this, SLOT(revert()));
    connect(video, SIGNAL(updateBtn()), this, SLOT(updateBtn()));
    connect(video, SIGNAL(updateProgressBar()), this, SLOT(updateProgressBar()));
    connect(video, SIGNAL(updateProcessProgress(std::string, int)), this, SLOT(updateProcessProgress(std::string, int)));
    connect(video, SIGNAL(closeProgressDialog()), this, SLOT(closeProgressDialog()));

    // amplify dialog
    magnifyDialog = new MagnifyDialog(this, video);
}

MainWindow::~MainWindow()
{
    delete ui;
}


/**
 * updateEnable	-	Update the enabled property of play menu and filter menu
 *
 * @param vi	-	the enable variable
 */
void MainWindow::updateStatus(bool vi)
{
    for (int i = 0; i < ui->menuPlay->actions().count(); ++i){
        ui->menuPlay->actions().at(i)->setEnabled(vi);
    }
    for (int i = 0; i < ui->menuProcessor->actions().count(); ++i){
        ui->menuProcessor->actions().at(i)->setEnabled(vi);
    }
    ui->actionClose->setEnabled(vi);
    ui->actionSave_as->setEnabled(vi);
    ui->progressSlider->setEnabled(vi);
    ui->btnLast->setEnabled(vi);;
    ui->btnNext->setEnabled(vi);
    ui->btnPlay->setEnabled(vi);
    ui->btnStop->setEnabled(vi);
    ui->btnPause->setEnabled(vi);
    ui->actionExtract_XT_Slice->setEnabled(vi);
    ui->actionExtract_YT_Slice->setEnabled(vi);
    ui->actionSave_Current_Frame->setEnabled(vi);

    if(!vi){
        ui->progressSlider->setValue(0);
        rateLabel->setText("");
    }
}

void MainWindow::updateTimeLabel()
{
    QString curPos = QDateTime::fromMSecsSinceEpoch(
                video->getPositionMS()).toString("hh:mm::ss");
    QString length = QDateTime::fromMSecsSinceEpoch(
                video->getLengthMS()).toString("hh:mm::ss");
    ui->timeLabel->setText(tr("<span style=' color:#ff0000;'>"
                              "%1</span> / %2").arg(curPos, length));
}

/** 
 * clean	-	clean all temp files
 *
 */
void MainWindow::clean()
{
    QString path = ".";
    QDir dir(path);

    std::string dirFile;

    while (true){
        video->getTempFile(dirFile);
        if (dirFile == "")
            break;
        else
            dir.remove(QString::fromStdString(dirFile));
    }

}


/** 
 * closeEvent	-	Do a file save check before close the application
 *
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        event->accept();        
        clean();
    } else {
        event->ignore();
    }
}


/** 
 * maybeSave	-	some thing to do before the next operation
 *
 * @return true if all preparation has done; false if the user cancel the operation.
 */
bool MainWindow::maybeSave()
{
    if (video->isModified()){
        // Create an warning dialog
        QMessageBox box;
        box.setWindowTitle(tr("Warning"));
        box.setIcon(QMessageBox::Warning);
        box.setText(tr("The current file %1 has been changed. Save?").arg(curFile));

        QPushButton *yesBtn = box.addButton(tr("Yes(&Y)"),
                                            QMessageBox::YesRole);
        box.addButton(tr("No(&N)"), QMessageBox::NoRole);
        QPushButton *cancelBut = box.addButton(tr("Cancel(&C)"),
                                               QMessageBox::RejectRole);

        box.exec();
        if (box.clickedButton() == yesBtn)
            return saveAs();
        if (box.clickedButton() == cancelBut)
            return false;
    }
    return true;
}

/** 
 * play	-	play the video
 *
 */
void MainWindow::play()
{
    video->setDelay(1000 / video->getFrameRate());
    video->playIt();
}


/** 
 * saveAs	-	Save the file as...
 *
 *
 * @return true if the file is successfully saved to the
 * 			user defined location
 */
bool MainWindow::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save as"),
                                                    curFile);

    if (fileName.isEmpty()) return false;
    return saveFile(fileName);
}

/** 
 * saveFile	-	Save the file to a specified location
 *
 * @param fileName	-	the target location
 *
 * @return true if the file is successfully saved
 */
bool MainWindow::saveFile(const QString &fileName)
{
    if (QFileInfo(fileName).suffix() == "") {
        video->setOutput(QFileInfo(fileName).filePath().toStdString(), ".avi");
    } else {
        video->setOutput(QFileInfo(fileName).filePath().toStdString());
    }

    // change the cursor
    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    // save all the contents to file
    video->writeOutput();

    // restore the cursor
    QApplication::restoreOverrideCursor();

    // set the current file location
    curFile = QFileInfo(fileName).canonicalPath();
    setWindowTitle(curFile);

    return true;
}


// save current frame
void MainWindow::on_actionSave_Current_Frame_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                        tr("Save as"),
                                                        curFile);
    if (fileName.isEmpty()) return;
    cv::Mat src;
    video->getNextFrame(src);
    saveImage(fileName, src);
}

/**
 * saveImage	-	Save the file to a specified location
 *
 * @param fileName	-	the target location
 *
 * @return true if the file is successfully saved
 */
bool MainWindow::saveImage(const QString &fileName, cv::Mat image)
{
    QString target;
    if (QFileInfo(fileName).suffix() == "")
        target = fileName + ".png";
    else
        target = fileName;
    QFile file(target);

    if (!file.open(QFile::WriteOnly)) {
        QMessageBox::warning(this, tr("ImageViewer"),
                             tr("Enable to save %1: \n %2")
                             .arg(target).arg(file.errorString()));
        return false;
    }

    // change the cursor
    QApplication::setOverrideCursor(Qt::WaitCursor);

    // save all the contents to file
    cv::imwrite(target.toStdString(), image);

    // restore the cursor
    QApplication::restoreOverrideCursor();

    // get the file's standard location
    curFile = QFileInfo(target).canonicalPath();
    setWindowTitle(curFile);

    return true;
}

/** 
 * loadFile	-	Open and load a video
 *
 * @param fileName	-	file location
 *
 * @return true if the file is successfully loaded
 */
bool MainWindow::LoadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::warning(this, tr("VideoPlayer"),
                             tr("Unable to load file %1:\n%2.")
                             .arg(fileName).arg(file.errorString()));
        return false;
    }

    // change the cursor
    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    // input file
    if (!video->setInput(fileName.toStdString())){
        QMessageBox::warning(this, tr("VideoPlayer"),
                             tr("Unable to load file %1:\n%2.")
                             .arg(fileName).arg(file.errorString()));
        return false;
    }

    // restore the cursor
    QApplication::restoreOverrideCursor();

    // update the frame rate
    rateLabel->setText(tr("Frame rate: %1").arg(video->getFrameRate()));

    // update the time label
    updateTimeLabel();

    magnifyDialog->enableCheck(false);
    selection = cv::Rect(0, 0, 0 ,0);
    video->setROI(selection);

    // set the current file location
    curFile = QFileInfo(fileName).canonicalPath();
    setWindowTitle(curFile);
    return true;
}


/** 
 * showImage	-	show a image
 *
 * @param image	-	the image to be showed
 *
 * Mainly used for display a single frame
 */
void MainWindow::showFrame(cv::Mat frame)
{
    cv::Mat tmp;
    cvtColor(frame, tmp, CV_BGR2RGB);
    QImage img = QImage((const unsigned char*)(tmp.data),
                        tmp.cols, tmp.rows, tmp.step, QImage::Format_RGB888);

    ui->videoLabel->setPixmap(QPixmap::fromImage(img));
    ui->videoLabel->repaint();
}


/** 
 * revert	-	do something when revert playing
 *
 */
void MainWindow::revert()
{
    updateBtn();
}

/** 
 * sleep	-	pause the video for several msecs
 *
 * @param msecs 
 */
void MainWindow::sleep(int msecs)
{
    helper->sleep(msecs);
}

/** 
 * updateBtn	-	update the button
 *
 */
void MainWindow::updateBtn()
{
    bool isStop = video->isStop();
    ui->actionPause->setVisible(!isStop);
    ui->btnPause->setVisible(!isStop);
    ui->actionPlay->setVisible(isStop);
    ui->btnPlay->setVisible(isStop);
}

/** 
 * updateProgressBar	-	update the progress bar
 *
 */
void MainWindow::updateProgressBar()
{
    // update the progress bar
    ui->progressSlider->setValue(video->getNumberOfPlayedFrames()
                                 * ui->progressSlider->maximum()
                                 / video->getLength() * 1.0);

    // update the time label
    updateTimeLabel();
}


/**
 * updateProgressBar	-	update the process progress dialog
 *
 */
void MainWindow::updateProcessProgress(const std::string &message, int value)
{
    if(!progressDialog){
        progressDialog = new QProgressDialog(this);
        progressDialog->setLabelText(QString::fromStdString(message));
        progressDialog->setRange(0, 100);
        progressDialog->setModal(true);
        progressDialog->setCancelButtonText(tr("Abort"));
        progressDialog->show();
        progressDialog->raise();
        progressDialog->activateWindow();
    }
    progressDialog->setValue(value + 1);
    qApp->processEvents();
    if (progressDialog->wasCanceled()){
        video->stopIt();
    }
}

/**
 * about	-	Display about info
 *
 */
void MainWindow::about()
{
    QMessageBox::about(this, tr("About QtEVM"),
                       tr("<h2>QtEVM 1.0</h2>"
                          "<p>Copyright &copy; 2014 Joseph Pan"
                          "(<a href=\"mailto:cs.wzpan@gmail.com\">"
                          "<span style=\" text-decoration: underline; color:#0000ff;\">"
                          "cs.wzpan@gmail.com</span></a>).</p>"
                          "<p>Yet anther C++ implementation of EVM"
                          "(<a href=\"http://people.csail.mit.edu/mrub/vidmag/\">"
                          "<span style=\" text-decoration: underline; color:#0000ff;\">"
                          "Eulerian Video Magnification</span></a>)"
                          ", based on OpenCV and Qt.</p>"));
}

// Open video file
void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open Video"),
                                                    ".",
                                                    tr("Video Files (*.avi *.mov *.mpg *.mpeg *.mp4)"));
    if(!fileName.isEmpty()) {
        if(LoadFile(fileName)){
            updateStatus(true);
            updateBtn();
        }
    }
}

// Quit
void MainWindow::on_actionQuit_triggered()
{
    // Execute the close operation before quit.
    on_actionClose_triggered();

    qApp->quit();
}

// Close video
void MainWindow::on_actionClose_triggered()
{
    if (maybeSave()) {
        updateStatus(false);
        ui->videoLabel->setText(inputTip);
        video->close();
        clean();
        ui->timeLabel->setText("");
        rateLabel->setText("");
    }
}

// Save As operation
void MainWindow::on_actionSave_as_triggered()
{
    saveAs();
}

// About
void MainWindow::on_actionAbout_triggered()
{
    about();
}

// About Qt
void MainWindow::on_actionAbout_Qt_triggered()
{
    qApp->aboutQt();
}

// stop action
void MainWindow::on_actionStop_S_triggered()
{
    video->stopIt();
}

// stop button action
void MainWindow::on_btnStop_clicked()
{
    video->stopIt();
}

// play button action
void MainWindow::on_btnPlay_clicked()
{
    play();
}

// play action
void MainWindow::on_actionPlay_triggered()
{
    play();
}

// pause action
void MainWindow::on_actionPause_triggered()
{
    video->pauseIt();
}

// pause button
void MainWindow::on_btnPause_clicked()
{
    video->pauseIt();
}

// next frame
void MainWindow::on_btnNext_clicked()
{
    video->nextFrame();
}

// prev frame
void MainWindow::on_btnLast_clicked()
{
    video->prevFrame();
}

// slider moved
void MainWindow::on_progressSlider_sliderMoved(int position)
{
    long pos = position * video->getLength() /
            ui->progressSlider->maximum();
    video->jumpTo(pos);

    updateTimeLabel();
}

// canceling the process
void MainWindow::closeProgressDialog()
{
    progressDialog->close();
    progressDialog = 0;
}

// Clean all the temp files
void MainWindow::on_actionClean_Temp_Files_triggered()
{
    QString path = ".";
    QDir dir(path);

    // set filter
    dir.setNameFilters(QStringList() << "temp*.avi");
    dir.setFilter(QDir::Files);
    std::string temp;

    // delete all the temp files
    foreach(QString dirFile, dir.entryList()){
        video->getCurTempFile(temp);
        if (dirFile != QString::fromStdString(temp))
            dir.remove(dirFile);
    }
}

// motion magnification
void MainWindow::on_motion_triggered()
{
    magnifyDialog->show();
    magnifyDialog->raise();
    magnifyDialog->activateWindow();

    if (magnifyDialog->exec() == QDialog::Accepted) {
        // change the cursor
        QApplication::setOverrideCursor(Qt::WaitCursor);
        // run the process
        video->motionMagnify();
        // restore the cursor
        QApplication::restoreOverrideCursor();
    }
}

// color (motion) magnification
void MainWindow::on_color_triggered()
{
    magnifyDialog->show();
    magnifyDialog->raise();
    magnifyDialog->activateWindow();

    if (magnifyDialog->exec() == QDialog::Accepted) {
        // change the cursor
        QApplication::setOverrideCursor(Qt::WaitCursor);
        // run the process
        video->colorMagnify();
        // restore the cursor
        QApplication::restoreOverrideCursor();
    }
}

// labeling
void MainWindow::on_actionLabeling_triggered()
{
    cv::Mat firstFrame;
    ForegroundExtractor *extractor = new ForegroundExtractor;
    video->getFirstFrame(firstFrame);
    frameWidth = firstFrame.cols;
    frameHeight = firstFrame.rows;

    cv::Size textSize1 = cv::getTextSize("Select a region for processing.", CV_FONT_HERSHEY_COMPLEX, 0.5, 1, 0);
    cv::Size textSize2 = cv::getTextSize("After selecting,", CV_FONT_HERSHEY_COMPLEX, 0.5, 1, 0);
    cv::Size textSize3 = cv::getTextSize("Press SPACE key to continue. Press ESC key to cancel.", CV_FONT_HERSHEY_COMPLEX, 0.5, 1, 0);
    cv::Size textSize4 = cv::getTextSize("Done selecting. Press any key to exit.", CV_FONT_HERSHEY_COMPLEX, 0.5, 1, 0);
    cv::Point org1((frameWidth - textSize1.width)/2, 30);
    cv::Point org2((frameWidth - textSize2.width)/2, 50);
    cv::Point org3((frameWidth - textSize3.width)/2, 70);
    cv::Point org4((frameWidth - textSize4.width)/2, 50);
    int lineType = 8;

    cv::Mat titleFrame1 = firstFrame.clone();

    cv::putText(titleFrame1, "Select a region for processing.", org1, CV_FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, lineType);
    cv::putText(titleFrame1, "After selecting,", org2, CV_FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, lineType);
    cv::putText(titleFrame1, "press SPACE key to finish; press ESC key to cancel.", org3, CV_FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 0, 255), 1, lineType);

    cv::namedWindow("Labeling Window");
    cv::imshow("Labeling Window", titleFrame1);
    cv::setMouseCallback( "Labeling Window", onMouse, 0 );
    cv::Mat temp = titleFrame1.clone();
    char c;
    while (true) {
        if (selection.area()) {
            temp = titleFrame1.clone();
            cv::Mat zone = cv::Mat(temp, selection);
            cv::Mat white(selection.size(), CV_8UC3, cv::Scalar(255, 255, 255));
            cv::addWeighted(white, 0.5, zone, 0.5, 0.0, zone);
            cv::imshow("Labeling Window", temp);
        }
        cv::imshow("Labeling Window", temp);
        c = cv::waitKey(10);
        if (32 == c){
            break;
        }
        if (27 == c){
            cv::destroyWindow("Labeling Window");
            return;
        }
    }
    if (trackObject){
        cv::Mat foreMask = extractor->getForegroundMask(firstFrame, selection);
        cv::Mat foreground = extractor->getForegroundWeighted(firstFrame, foreMask, 0.5);
        cv::rectangle(foreground, selection, cv::Scalar(0, 255, 0));
        cv::putText(foreground, "Done selecting. Press any key to exit.", org4, CV_FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(255, 0, 0), 1, lineType);
        cv::imshow("Labeling Window", foreground);
        video->setROI(selection);
        magnifyDialog->enableCheck(true);
        cv::waitKey(0);
        cv::destroyWindow("Labeling Window");
    }
}

// extract a YT slice
void MainWindow::on_actionExtract_YT_Slice_triggered()
{
    cv::Mat firstFrame;
    video->getFirstFrame(firstFrame);
    frameWidth = firstFrame.cols;
    frameHeight = firstFrame.rows;

    cv::Size textSize1 = cv::getTextSize("Select a x position for processing.", CV_FONT_HERSHEY_COMPLEX, 0.5, 1, 0);
    cv::Size textSize2 = cv::getTextSize("After selecting,", CV_FONT_HERSHEY_COMPLEX, 0.5, 1, 0);
    cv::Size textSize3 = cv::getTextSize("Press SPACE key to continue. Press ESC key to cancel.", CV_FONT_HERSHEY_COMPLEX, 0.5, 1, 0);
    cv::Size textSize4 = cv::getTextSize("Press SPACE key to save, other key to exit.", CV_FONT_HERSHEY_COMPLEX, 0.5, 1, 0);
    cv::Point org1((frameWidth - textSize1.width)/2, 30);
    cv::Point org2((frameWidth - textSize2.width)/2, 50);
    cv::Point org3((frameWidth - textSize3.width)/2, 70);
    int lineType = 8;

    cv::Mat titleFrame1 = firstFrame.clone();

    cv::putText(titleFrame1, "Select a x position for processing.", org1, CV_FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, lineType);
    cv::putText(titleFrame1, "After selecting,", org2, CV_FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, lineType);
    cv::putText(titleFrame1, "Press SPACE key to continue. Press ESC key to cancel.", org3, CV_FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 0, 255), 1, lineType);

    cv::namedWindow("Extracting Window");
    cv::imshow("Extracting Window", titleFrame1);
    cv::setMouseCallback( "Extracting Window", onMouseX, 0 );
    cv::Mat temp = titleFrame1.clone();
    char c;
    while (true) {
        if (sliceX) {
            temp = titleFrame1.clone();
            cv::line(temp, cv::Point(sliceX, 0), cv::Point(sliceX, frameHeight), cv::Scalar(0, 255, 0));
            cv::imshow("Extracting Window", temp);
        }
        cv::imshow("Extracting Window", temp);
        c = cv::waitKey(10);
        if (32 == c){
            break;
        }
        if (27 == c){
            cv::destroyWindow("Extracting Window");
            return;
        }
    }

    if (sliceX){
        cv::Mat slice = video->extractYTSlice(sliceX);
        cv::Mat titleSlice = slice.clone();

        cv::line(titleSlice, cv::Point(sliceX, 0), cv::Point(sliceX, frameHeight), cv::Scalar(0, 255, 0));
        cv::Point org4((slice.cols - textSize4.width)/2, 50);
        cv::putText(titleSlice, "Press SPACE key to save, other key to exit.", org4, CV_FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(255, 0, 0), 1, lineType);
        cv::imshow("Extracting Window", titleSlice);

        while (true) {
            c = cv::waitKey(10);
            if (32 == c){
                QString fileName = QFileDialog::getSaveFileName(this,
                                                                    tr("Save as"),
                                                                    curFile);
                if (fileName.isEmpty()) return;
                saveImage(fileName, slice);
                cv::destroyWindow("Extracting Window");
            }
            if (27 == c){
                cv::destroyWindow("Extracting Window");
                return;
            }
        }
    }
}

void MainWindow::on_actionExtract_XT_Slice_triggered()
{
    cv::Mat firstFrame;
    video->getFirstFrame(firstFrame);
    frameWidth = firstFrame.cols;
    frameHeight = firstFrame.rows;

    cv::Size textSize1 = cv::getTextSize("Select a y position for processing.", CV_FONT_HERSHEY_COMPLEX, 0.5, 1, 0);
    cv::Size textSize2 = cv::getTextSize("After selecting,", CV_FONT_HERSHEY_COMPLEX, 0.5, 1, 0);
    cv::Size textSize3 = cv::getTextSize("Press SPACE key to continue. Press ESC key to cancel.", CV_FONT_HERSHEY_COMPLEX, 0.5, 1, 0);
    cv::Size textSize4 = cv::getTextSize("Press SPACE key to save, other key to exit.", CV_FONT_HERSHEY_COMPLEX, 0.5, 1, 0);
    cv::Point org1((frameWidth - textSize1.width)/2, 30);
    cv::Point org2((frameWidth - textSize2.width)/2, 50);
    cv::Point org3((frameWidth - textSize3.width)/2, 70);
    int lineType = 8;

    cv::Mat titleFrame1 = firstFrame.clone();

    cv::putText(titleFrame1, "Select a y position for processing.", org1, CV_FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, lineType);
    cv::putText(titleFrame1, "After selecting,", org2, CV_FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, lineType);
    cv::putText(titleFrame1, "Press SPACE key to continue. Press ESC key to cancel.", org3, CV_FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 0, 255), 1, lineType);

    cv::namedWindow("Extracting Window");
    cv::imshow("Extracting Window", titleFrame1);
    cv::setMouseCallback( "Extracting Window", onMouseY, 0 );
    cv::Mat temp = titleFrame1.clone();
    char c;
    while (true) {
        if (sliceY) {
            temp = titleFrame1.clone();
            cv::line(temp, cv::Point(0, sliceY), cv::Point(frameWidth, sliceY), cv::Scalar(0, 255, 0));
            cv::imshow("Extracting Window", temp);
        }
        cv::imshow("Extracting Window", temp);
        c = cv::waitKey(10);
        if (32 == c){
            break;
        }
        if (27 == c){
            cv::destroyWindow("Extracting Window");
            return;
        }
    }

    if (sliceY){
        cv::Mat slice = video->extractXTSlice(sliceY);
        cv::Mat titleSlice = slice.clone();

        cv::line(titleSlice, cv::Point(0, sliceY), cv::Point(frameWidth, sliceY), cv::Scalar(0, 255, 0));
        cv::Point org4((slice.cols - textSize4.width)/2, 50);
        cv::putText(titleSlice, "Press SPACE key to save, other key to exit.", org4, CV_FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(255, 0, 0), 1, lineType);
        cv::imshow("Extracting Window", titleSlice);

        while (true) {
            c = cv::waitKey(10);
            if (32 == c){
                QString fileName = QFileDialog::getSaveFileName(this,
                                                                    tr("Save as"),
                                                                    curFile);
                if (fileName.isEmpty()) return;
                saveImage(fileName, slice);
                cv::destroyWindow("Extracting Window");
            }
            if (27 == c){
                cv::destroyWindow("Extracting Window");
                return;
            }
        }
    }
}
