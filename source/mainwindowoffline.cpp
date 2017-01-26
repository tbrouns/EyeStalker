//  Copyright (C) 2016  Terence Brouns

//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.

//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>

#include "headers/mainwindow.h"

void MainWindow::startOfflineSession()
{
    trialIndexOffline = 0;
    imageIndexOffline = 0;
    Parameters::ONLINE_PROCESSING = false; // turn off pupil tracking

    { // unlock frame grabbing threads
        std::unique_lock<std::mutex> frameCaptureMutexLock(Parameters::frameCaptureMutex);
        Parameters::frameCaptureCV.notify_all(); // unlock getFrame() thread
    }

    { // wait for threads to finish
        std::unique_lock<std::mutex> lck(mtx);
        while (Parameters::CAMERA_RUNNING) cv.wait(lck);
    }

    // hide camera AOI sliders
    CamEyeAOIWdthSlider->setVisible(false);
    CamEyeAOIHghtSlider->setVisible(false);
    CamEyeAOIXPosSlider->setVisible(false);
    CamEyeAOIYPosSlider->setVisible(false);

    OfflineModeMainWidget  ->setVisible(true);
    OfflineModeHeaderWidget->setVisible(true);

    CamQImage->clearImage();
    EyeQImage->clearImage();

    // hide tab with camera parameters
    MainTabWidget->removeTab(0);

    setupOfflineSession();
}

void MainWindow::countNumTrials()
{
    trialTotalOffline = 0;

    while (1)
    {
        std::stringstream folderName;
        folderName << dataDirectoryOffline.toStdString()
                   << "/trial_"
                   << trialTotalOffline;
        if (!boost::filesystem::exists(folderName.str())) { break; }
        trialTotalOffline++;
    }

    OfflineTrialSlider ->setMaximum(trialTotalOffline - 1);
    OfflineTrialSpinBox->setMaximum(trialTotalOffline - 1);
}

void MainWindow::countNumImages()
{
    imageTotalOffline = 0;

    while (1)
    {
        std::stringstream filename;
        filename << dataDirectoryOffline.toStdString()
                 << "/trial_"
                 << trialIndexOffline
                 << "/raw/"
                 << imageTotalOffline
                 << ".png";

        if (!boost::filesystem::exists(filename.str())) { break; }
        else { imageTotalOffline++; }
    }
}


void MainWindow::onLoadSession()
{
    dataDirectoryOffline = QFileDialog::getExistingDirectory(this, tr("Select data directory"), "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dataDirectoryOffline.isEmpty())
    {
        timeMatrix.clear();
        setupOfflineSession();
    }
}

void MainWindow::setupOfflineSession()
{
    if (!dataDirectoryOffline.isEmpty())
    {
        countNumTrials();

        if (trialTotalOffline > 0)
        {
            if (trialIndexOffline != 0) { OfflineTrialSlider->setValue(0); } // start with first trial
            else { onSetTrialIndexOffline(0); }

            std::stringstream directoryName;
            directoryName << dataDirectoryOffline.toStdString()
                          << "/trial_"
                          << trialIndexOffline
                          << "/processed";

            if (!boost::filesystem::exists(directoryName.str()))
            {    boost::filesystem::create_directory(directoryName.str().c_str()); }

            if (timeMatrix.empty()) // Grab time stamps
            {
                std::stringstream directory;
                directory << dataDirectoryOffline.toStdString()
                          << "/timestamps.dat";

                std::ifstream data;
                data.open(directory.str().c_str());

                if (data.is_open())
                {
                    std::string str;

                    for (int i = 0; i < trialTotalOffline && std::getline(data, str); i++)
                    {
                        std::vector<double> times;
                        std::istringstream sin(str);
                        double time;
                        while (sin >> time) { times.push_back(time); }
                        timeMatrix.push_back(times);
                    }
                }
            }
        }
    }
}

void MainWindow::onSetTrialIndexOffline(int index)
{
    trialIndexOffline = index;

    countNumImages();

    if (imageTotalOffline > 0)
    {
        vDetectionMiscellaneousEye.resize(imageTotalOffline);
        vDetectionVariablesEye.resize(imageTotalOffline + 1);
        vDetectionVariablesBead.resize(imageTotalOffline + 1);

        mVariableWidgetEye ->resetStructure(mParameterWidgetEye ->getStructure());
        mVariableWidgetBead->resetStructure(mParameterWidgetBead->getStructure());

        vDetectionVariablesEye[0]  = mVariableWidgetEye ->getStructure();
        vDetectionVariablesBead[0] = mVariableWidgetBead->getStructure();

        if (imageIndexOffline != 0) { OfflineImageSlider->setValue(0); } // start with first frame
        else { onSetOfflineImage(0); }

        OfflineImageSlider->setMaximum(imageTotalOffline - 1);

        // Create folder

        std::stringstream directoryPath;
        directoryPath << dataDirectoryOffline.toStdString()
                      << "/trial_"
                      << trialIndexOffline
                      << "/processed/";

        if (!boost::filesystem::exists(directoryPath.str()))
        {    boost::filesystem::create_directory(directoryPath.str().c_str()); }
    }
}

void MainWindow::updateImageRaw(int imgIndex) // for signal from qimageopencv
{
    if (imgIndex < 0) { imgIndex = imageIndexOffline; }

    std::stringstream imagePath;
    imagePath << dataDirectoryOffline.toStdString()
              << "/trial_"
              << trialIndexOffline
              << "/raw/"
              << imgIndex
              << ".png";

    if (boost::filesystem::exists(imagePath.str()))
    {
        cv::Mat eyeImageRaw = cv::imread(imagePath.str(), CV_LOAD_IMAGE_COLOR);
        CamQImage->loadImage(eyeImageRaw);
        { std::lock_guard<std::mutex> mainMutexLock(Parameters::mainMutex);
            Parameters::cameraAOIWdth = eyeImageRaw.cols;
            Parameters::cameraAOIHght = eyeImageRaw.rows;
            updateEyeAOIx();
            updateEyeAOIy();
            CamQImage->setAOIEye ( Parameters::eyeAOIXPos,  Parameters::eyeAOIYPos,  Parameters::eyeAOIWdth,  Parameters::eyeAOIHght);
            CamQImage->setAOIBead(Parameters::beadAOIXPos, Parameters::beadAOIYPos, Parameters::beadAOIWdth, Parameters::beadAOIHght);
        }
        CamQImage->setImage();
    } else { CamQImage->clearImage(); }
}

void MainWindow::updateImageProcessed(int imgIndex)
{
    std::stringstream fileName;
    fileName << dataDirectoryOffline.toStdString()
             << "/trial_"
             << trialIndexOffline
             << "/processed/"
             << imgIndex
             << ".png";

    if (boost::filesystem::exists(fileName.str()))
    {
        cv::Mat eyeImage = cv::imread(fileName.str(), CV_LOAD_IMAGE_COLOR);
        EyeQImage->loadImage(eyeImage);
        EyeQImage->setImage();
    }
    else
    {
        EyeQImage->clearImage();
    }
}

void MainWindow::onImagePrevious()
{
    if (imageIndexOffline > 0)
    {
        imageIndexOffline--;
        OfflineImageSlider->setValue(imageIndexOffline);
    }
}

void MainWindow::onImageNext()
{
    if (imageIndexOffline < imageTotalOffline - 1)
    {
        imageIndexOffline++;
        OfflineImageSlider->setValue(imageIndexOffline);
    }
}

void MainWindow::onSetOfflineImage(int imgIndex)
{
    if (!PROCESSING_ALL_IMAGES) { imageIndexOffline = imgIndex; }

    { std::lock_guard<std::mutex> mainMutexLock(Parameters::mainMutex);
        mVariableWidgetEye ->setWidgets(vDetectionVariablesEye [imgIndex]);
        mVariableWidgetBead->setWidgets(vDetectionVariablesBead[imgIndex]);
    }

    if (imageTotalOffline > 0)
    {
        std::stringstream ss;
        ss << "<b>" << imgIndex + 1 << " / " << imageTotalOffline << "</b>";
        QString title = QString::fromStdString(ss.str());
        OfflineImageFrameTextBox->setText(title);

        updateImageRaw(imgIndex);
        updateImageProcessed(imgIndex);
    }
    else
    {
        CamQImage->clearImage();
        EyeQImage->clearImage();
        OfflineImageFrameTextBox->setText("<b>0 / 0</b>");
    }
}

void MainWindow::detectCurrentFrame(int imageIndex)
{
    // Grab raw images

    cv::Mat imageRaw;

    std::stringstream imagePathRaw;
    imagePathRaw << dataDirectoryOffline.toStdString()
                 << "/trial_"
                 << trialIndexOffline
                 << "/raw/"
                 << imageIndex
                 << ".png";

    if (boost::filesystem::exists(imagePathRaw.str())) { imageRaw = cv::imread(imagePathRaw.str(), CV_LOAD_IMAGE_COLOR); }
    else                                               { return; }

    // Detect pupil

    int cameraAOIXPos;
    int cameraAOIYPos;

    detectionProperties mDetectionPropertiesEye;
    detectionProperties mDetectionPropertiesBead;

    { std::lock_guard<std::mutex> mainMutexLock(Parameters::mainMutex);

        mDetectionPropertiesEye.v = vDetectionVariablesEye[imageIndex];
        mDetectionPropertiesEye.p = mParameterWidgetEye->getStructure();

        mDetectionPropertiesBead.v = vDetectionVariablesBead[imageIndex];
        mDetectionPropertiesBead.p = mParameterWidgetBead->getStructure();

        cameraAOIXPos = imageRaw.cols;
        cameraAOIYPos = imageRaw.rows;

        Parameters::cameraAOIWdth = cameraAOIXPos;
        Parameters::cameraAOIHght = cameraAOIYPos;

        updateEyeAOIx();
        updateEyeAOIy();

        mDetectionPropertiesEye.p.AOIXPos = Parameters::eyeAOIXPos;
        mDetectionPropertiesEye.p.AOIYPos = Parameters::eyeAOIYPos;
        mDetectionPropertiesEye.p.AOIWdth = Parameters::eyeAOIWdth;
        mDetectionPropertiesEye.p.AOIHght = Parameters::eyeAOIHght;

        mDetectionPropertiesBead.p.AOIXPos = Parameters::beadAOIXPos;
        mDetectionPropertiesBead.p.AOIYPos = Parameters::beadAOIYPos;
        mDetectionPropertiesBead.p.AOIWdth = Parameters::beadAOIWdth;
        mDetectionPropertiesBead.p.AOIHght = Parameters::beadAOIHght;
    }

    detectionProperties mDetectionPropertiesNewEye = pupilDetection(imageRaw, mDetectionPropertiesEye);

    cv::Mat imageProcessed = imageRaw.clone();
    drawAll(imageProcessed, mDetectionPropertiesNewEye);

    detectionProperties mDetectionPropertiesNewBead;
    if (mParameterWidgetBead->getState())
    {
        mDetectionPropertiesNewBead = pupilDetection(imageRaw, mDetectionPropertiesBead);
        drawAll(imageProcessed, mDetectionPropertiesNewBead);
    }

    // Save processed images

    std::stringstream imagePath;
    imagePath << dataDirectoryOffline.toStdString()
              << "/trial_"
              << trialIndexOffline
              << "/processed/"
              << imageIndex
              << ".png";

    cv::imwrite(imagePath.str(), imageProcessed);

    // Get pupil coords in screen coords

    mDetectionPropertiesNewEye.v.xPosAbsolute = mDetectionPropertiesNewEye.v.xPosExact + cameraAOIXPos;
    mDetectionPropertiesNewEye.v.yPosAbsolute = mDetectionPropertiesNewEye.v.yPosExact + cameraAOIYPos;

    // Record pupil positions

    { std::lock_guard<std::mutex> mainMutexLock(Parameters::mainMutex);
        vDetectionVariablesEye[imageIndex + 1] = mDetectionPropertiesNewEye.v;
        vDetectionMiscellaneousEye[imageIndex] = mDetectionPropertiesNewEye.m;

        if (mParameterWidgetBead->getState())
        {
            vDetectionVariablesBead[imageIndex + 1] = mDetectionPropertiesNewBead.v;
        }
    }
}

void MainWindow::onDetectCurrentFrame()
{
    detectCurrentFrame(imageIndexOffline);
    updateImageProcessed(imageIndexOffline);
}

void MainWindow::onDetectAllFrames()
{
    if (!PROCESSING_ALL_IMAGES)
    {
        PROCESSING_ALL_IMAGES = true;
        OFFLINE_SAVE_DATA     = false;

        std::thread pupilDetectionThread(&MainWindow::detectAllFrames, this);
        pupilDetectionThread.detach();

        while (!OFFLINE_SAVE_DATA && PROCESSING_ALL_IMAGES)
        {
            std::stringstream ss;
            ss << "<b>" << imageIndexOffline + 1 << " / " << imageTotalOffline << "</b>";
            QString title = QString::fromStdString(ss.str());
            OfflineImageFrameTextBox->setText(title);

            OfflineImageSlider->setValue(imageIndexOffline - 1); // Show progress

            msWait(1000 / guiUpdateFrequency);
        }
    }

    {
        std::unique_lock<std::mutex> lck(mtxOffline);
        PROCESSING_ALL_IMAGES = false;
        cvOffline.notify_one(); // notify save-thread that (this) main-thread has exited while loop
    }
    {
        std::unique_lock<std::mutex> lck(mtxOffline);
        while (OFFLINE_SAVE_DATA) cvOffline.wait(lck); // wait for save-thread to complete saving
    }
}

void MainWindow::detectAllFrames()
{
    int initialIndex = imageIndexOffline; // needed for progressbar

    for (imageIndexOffline = initialIndex; imageIndexOffline < imageTotalOffline && PROCESSING_ALL_IMAGES; imageIndexOffline++)
    {
        detectCurrentFrame(imageIndexOffline);
    }

    imageIndexOffline--; // for-loop overshoots value

    if (PROCESSING_ALL_IMAGES)
    {
        OFFLINE_SAVE_DATA = true;

        { std::unique_lock<std::mutex> lck(mtxOffline);
            while (PROCESSING_ALL_IMAGES) cvOffline.wait(lck); } // wait for main-thread to exit while loop

        onSaveTrialData();

        { std::unique_lock<std::mutex> lck(mtxOffline);
            OFFLINE_SAVE_DATA = false;
            cvOffline.notify_one(); } // notify main-thread that saving has been completed

        OfflineImageSlider->setValue(imageIndexOffline);
    }
}

void MainWindow::onDetectAllTrials()
{
    if (!PROCESSING_ALL_TRIALS)
    {
        PROCESSING_ALL_TRIALS = true;

        for (int iTrial = trialIndexOffline; iTrial < trialTotalOffline && PROCESSING_ALL_TRIALS; iTrial++)
        {
            OfflineTrialSlider->setValue(iTrial);
            onDetectAllFrames();
        }
    }

    PROCESSING_ALL_IMAGES = false;
    PROCESSING_ALL_TRIALS = false;
}

void MainWindow::onSaveTrialData()
{
    std::string delimiter = ";";

    { // save pupil data

        std::stringstream filename;
        filename << dataDirectoryOffline.toStdString()
                 << "/trial_"
                 << trialIndexOffline
                 << "/tracking_data.dat";

        std::ofstream file;
        file.open(filename.str(), std::ios::trunc); // open file and remove any existing data

        if (timeMatrix.size() > 0) { file << std::setw(3) << std::setfill('0') << timeMatrix[trialIndexOffline][0] << ";"; } // trial index
        file << imageTotalOffline << ";";  // data samples
        if (timeMatrix.size() > 0) { file << (int) timeMatrix[trialIndexOffline][1] << ";"; } // system clock time

        file << std::fixed;
        file << std::setprecision(3);

        // write data

        for (int i = 0; i < imageTotalOffline; i++) { file << vDetectionVariablesEye[i + 1].pupilDetected << delimiter; }
        if (timeMatrix.size() > 0) { for (int i = 0; i < imageTotalOffline; i++) { file << timeMatrix[trialIndexOffline][i + 2] << delimiter; } }

        if (SAVE_POSITION)
        {
            for (int i = 0; i < imageTotalOffline; i++) { file << vDetectionVariablesEye[i + 1].xPosAbsolute  << delimiter; }
            for (int i = 0; i < imageTotalOffline; i++) { file << vDetectionVariablesEye[i + 1].yPosAbsolute  << delimiter; }
        }

        if (SAVE_CIRCUMFERENCE)
        {
            for (int i = 0; i < imageTotalOffline; i++) { file << vDetectionVariablesEye[i + 1].circumferenceExact << delimiter; }
        }

        if (SAVE_ASPECT_RATIO)
        {
            for (int i = 0; i < imageTotalOffline; i++) { file << vDetectionVariablesEye[i + 1].aspectRatioExact << delimiter; }
        }

        for (int i = 0; i < imageTotalOffline; i++) { file << vDetectionVariablesEye[i].edgeCurvaturePrediction << delimiter; }
        for (int i = 0; i < imageTotalOffline; i++) { file << vDetectionVariablesEye[i].edgeIntensityPrediction << delimiter; }

        file.close();
    }

    { // save edge data

        std::stringstream filename;
        filename << dataDirectoryOffline.toStdString()
                 << "/trial_"
                 << trialIndexOffline
                 << "/edge_data.dat";

        std::ofstream file;
        file.open(filename.str());

        for (int i = 0; i < imageTotalOffline; i++)
        {
            int numEdges = vDetectionMiscellaneousEye[i].edgePropertiesAll.size();

            for (int j = 0; j < numEdges; j++) { file << vDetectionMiscellaneousEye[i].edgePropertiesAll[j].flag         << delimiter; }
            for (int j = 0; j < numEdges; j++) { file << vDetectionMiscellaneousEye[i].edgePropertiesAll[j].curvatureMax << delimiter; }
            for (int j = 0; j < numEdges; j++) { file << vDetectionMiscellaneousEye[i].edgePropertiesAll[j].curvatureMin << delimiter; }
            for (int j = 0; j < numEdges; j++) { file << vDetectionMiscellaneousEye[i].edgePropertiesAll[j].curvatureAvg << delimiter; }
            for (int j = 0; j < numEdges; j++) { file << vDetectionMiscellaneousEye[i].edgePropertiesAll[j].length       << delimiter; }
            for (int j = 0; j < numEdges; j++) { file << vDetectionMiscellaneousEye[i].edgePropertiesAll[j].size         << delimiter; }
            for (int j = 0; j < numEdges; j++) { file << vDetectionMiscellaneousEye[i].edgePropertiesAll[j].distance     << delimiter; }
            for (int j = 0; j < numEdges; j++) { file << vDetectionMiscellaneousEye[i].edgePropertiesAll[j].intensity    << delimiter; }
            file << "\n";
        }

        file.close();
    }

    if (SAVE_PUPIL_IMAGE)
    {
        std::stringstream directoryName;
        directoryName << dataDirectoryOffline.toStdString()
                      << "/trial_"
                      << trialIndexOffline
                      << "/pupil";

        if (!boost::filesystem::exists(directoryName.str()))
        {
            boost::filesystem::create_directory(directoryName.str().c_str());
        }

        for (int i = 0; i < imageTotalOffline; i++)
        {
            if (vDetectionVariablesEye[i + 1].pupilDetected)
            {
                std::stringstream filename;
                filename << dataDirectoryOffline.toStdString()
                         << "/trial_"
                         << trialIndexOffline
                         << "/pupil/"
                         << i
                         << ".png";

                std::vector<int> compression_params;
                compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
                compression_params.push_back(0);

                cv::imwrite(filename.str(), vDetectionMiscellaneousEye[i].imagePupil, compression_params);
            }
        }
    }
}

void MainWindow::onCombineData()
{
    dataFilename = (DataFilenameLineEdit->text()).toStdString();
    std::stringstream fileNameWriteSS;
    fileNameWriteSS << dataDirectoryOffline.toStdString()
                    << "/"
                    << dataFilename
                    << ".dat";

    std::string fileNameWrite = fileNameWriteSS.str();

    if (boost::filesystem::exists(fileNameWrite))
    {
        QString text = "The file <b>" + QString::fromStdString(dataFilename) + ".dat</b> already exists in <b>" + dataDirectoryOffline + "/</b>. Do you wish to add data to the end of this file?";
        ConfirmationWindow mConfirmationWindow(text);
        mConfirmationWindow.setWindowTitle("Please select option");

        if(mConfirmationWindow.exec() == QDialog::Rejected) { return; }
    }

    for (int iTrial = 0; iTrial < trialTotalOffline; iTrial++)
    {
        OfflineTrialSlider->setValue(iTrial);

        std::stringstream fileNameRead;
        fileNameRead << dataDirectoryOffline.toStdString()
                     << "/trial_"
                     << iTrial
                     << "/tracking_data.dat";

        std::ifstream dataFile;
        dataFile.open(fileNameRead.str().c_str());

        std::string line;
        while (dataFile) { if (!std::getline(dataFile, line)) { break; }}

        std::ofstream file;
        file.open(fileNameWrite, std::ios_base::app);
        file << line;
        file << "\n";
        file.close();
    }
}


