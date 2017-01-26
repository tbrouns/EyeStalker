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

#include "headers/qimageopencv.h"

QImageOpenCV::QImageOpenCV(int type, QWidget *parent) : QLabel(parent)
{    
    imageType = type;

    spinnerDegrees = 0;

    imageWdth = 0;
    imageHght = 0;

    widgetWdth = 0;
    widgetHght = 0;

    backgroundColour = QColor( 48,  47,  47);
    textColour       = QColor(177, 177, 177);

    this->setLineWidth(2);
    this->setAlignment(Qt::AlignCenter);
    this->setFrameStyle(QFrame::Panel | QFrame::Raised);
}

QImageOpenCV::~QImageOpenCV()
{

}

void QImageOpenCV::setSize(int W, int H)
{
    widgetWdth = W;
    widgetHght = H;
    aspectRatio = W / (double) H;
}

QSize QImageOpenCV::sizeHint() const
{
    return QSize(widgetWdth, widgetHght);
}

void QImageOpenCV::loadImage(const cv::Mat& cvimage)
{
    this->clear();

    // Copy input Mat
    const uchar *qImageBuffer = (const uchar*)cvimage.data;

    // Create QImage with same dimensions as input Mat
    QImage img(qImageBuffer, cvimage.cols, cvimage.rows, cvimage.step, QImage::Format_RGB888);

    image = QPixmap::fromImage(img.rgbSwapped());

    imageWdth = image.width();
    imageHght = image.height();

    resizeImage();
}

void QImageOpenCV::resizeImage()
{
    if (imageWdth > widgetWdth || imageHght > widgetHght)
    {
        imageScaled = image.scaled(QSize(widgetWdth, widgetHght), Qt::KeepAspectRatio);

        imageWdthScaled = imageScaled.width();
        imageHghtScaled = imageScaled.height();

        imageScaleFactorX = imageWdthScaled / (double) imageWdth;
        imageScaleFactorY = imageHghtScaled / (double) imageHght;
    }
    else
    {
        imageScaled = image;

        imageWdthScaled = imageWdth;
        imageHghtScaled = imageHght;

        imageScaleFactorX = 1.0;
        imageScaleFactorY = 1.0;
    }
}

void QImageOpenCV::drawAOI(QPixmap& img, int xPos, int yPos, int wdth, int hght, QColor col)
{
    if (imageWdthScaled > 0 && wdth > 0 && hght > 0)
    {
        xPos = round(imageScaleFactorX * xPos);
        wdth = round(imageScaleFactorX * wdth);

        yPos = round(imageScaleFactorY * yPos);
        hght = round(imageScaleFactorY * hght);

        QPainter painter(&img);

        int lineWidth = round(0.003 * (widgetWdth + widgetHght));

        painter.setPen(QPen(col, round(lineWidth)));

        painter.drawLine(QPoint(xPos, yPos),        QPoint(xPos + wdth, yPos));
        painter.drawLine(QPoint(xPos, yPos + hght), QPoint(xPos + wdth, yPos + hght));

        painter.setPen(QPen(col, round(lineWidth)));

        painter.drawLine(QPoint(xPos,        yPos), QPoint(xPos,        yPos + hght));
        painter.drawLine(QPoint(xPos + wdth, yPos), QPoint(xPos + wdth, yPos + hght));
    }
}

void QImageOpenCV::setImage()
{
    if (imageType == 1)
    {
        if (imageWdthScaled > 0)
        {
            QPixmap imageEdited = imageScaled;
            drawAOI(imageEdited,   eyeXPosAOI,   eyeYPosAOI,   eyeWdthAOI,   eyeHghtAOI, QColor(255,   0,   0));
            drawAOI(imageEdited, flashXPosAOI, flashYPosAOI, flashWdthAOI, flashHghtAOI, QColor(  0,   0, 255));
            drawAOI(imageEdited,  beadXPosAOI,  beadYPosAOI,  beadWdthAOI,  beadHghtAOI, QColor(  0, 255,   0));
            this->setPixmap(imageEdited);
        }
    }
    else if (imageWdthScaled > 0) { this->setPixmap(imageScaled); }
}

void QImageOpenCV::clearImage()
{
    this->clear();
}

void QImageOpenCV::setFindingCamera()
{
    QPixmap pic(widgetWdth, widgetHght);
    pic.fill(backgroundColour);

    QRect rec(0, 0, round(0.9 * widgetWdth), round(0.5 * widgetHght));
    rec.moveCenter(rect().center());

    QPainter painter(&pic);

    QFont font = painter.font();
    font.setPointSize(round(0.032 * widgetWdth));
    font.setWeight(QFont::DemiBold);
    painter.setFont(font);

    painter.setPen(textColour);
    painter.drawText(rec, Qt::AlignCenter, QString("NO CAMERA DETECTED"));
    this->setPixmap(pic);
}

void QImageOpenCV::setAOIEye(int x, int y, int w, int h)
{
    eyeXPosAOI = x;
    eyeYPosAOI = y;
    eyeWdthAOI = w;
    eyeHghtAOI = h;
}

void QImageOpenCV::setAOIBead(int x, int y, int w, int h)
{
    beadXPosAOI = x;
    beadYPosAOI = y;
    beadWdthAOI = w;
    beadHghtAOI = h;
}

void QImageOpenCV::setAOIFlash(int x, int y, int w, int h)
{
    flashXPosAOI = x - Parameters::cameraAOIXPos;
    flashYPosAOI = y - Parameters::cameraAOIYPos;
    flashWdthAOI = w;
    flashHghtAOI = h;

    if (flashXPosAOI < 0)
    {
        if (flashXPosAOI + flashWdthAOI < 0) { flashWdthAOI = 0; }
        else
        {
            flashWdthAOI = flashXPosAOI + flashWdthAOI;
            flashXPosAOI = 0;
        }
    }

    if (flashYPosAOI < 0)
    {
        if (flashYPosAOI + flashHghtAOI < 0) { flashHghtAOI = 0; }
        else
        {
            flashHghtAOI = flashYPosAOI + flashHghtAOI;
            flashYPosAOI = 0;
        }
    }
}



void QImageOpenCV::setAOIError()
{
    QPixmap pic(widgetWdth, widgetHght);
    pic.fill(backgroundColour);

    QRect rec(0, 0, round(0.9 * widgetWdth), round(0.5 * widgetHght));
    rec.moveCenter(rect().center());

    QPainter painter(&pic);

    QFont font = painter.font();
    font.setPointSize(round(0.032 * widgetWdth));
    font.setWeight(QFont::DemiBold);
    painter.setFont(font);

    painter.setPen(textColour);
    painter.drawText(rec, Qt::AlignCenter, QString("AREA OF INTEREST TOO SMALL"));
    this->setPixmap(pic);
}

void QImageOpenCV::setSpinner()
{
    QPixmap pic(widgetWdth, widgetHght);
    pic.fill(backgroundColour);

    QRect rec(0, 0, round(0.4 * widgetHght), round(0.4 * widgetHght));
    rec.moveCenter(rect().center());

    QPainter painter(&pic);

    QConicalGradient gradient;
    gradient.setCenter(rec.center());
    gradient.setAngle(round(spinnerDegrees / 16));
    gradient.setColorAt(0, backgroundColour);
    gradient.setColorAt(1, Qt::white);

    QPen pen(QBrush(gradient), 2);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);

    painter.drawArc(rec, spinnerDegrees, 16 * 360); // each step is 1/16th of a degree

    this->setPixmap(pic);

    spinnerDegrees = (spinnerDegrees % (360 * 16)) + 180;
}

void QImageOpenCV::mousePressEvent(QMouseEvent *event)
{
    if (imageType == 1)
    {
        int imageScaledXOffset = 0.5 * (widgetWdth - imageWdthScaled);
        int imageScaledYOffset = 0.5 * (widgetHght - imageHghtScaled);

        if (event->button() == Qt::LeftButton)
        {
            { std::lock_guard<std::mutex> mainMutexLock(Parameters::mainMutex);

                double mouseXPos = (event->x()) - imageScaledXOffset;

                Parameters::eyeAOIXPos = round(mouseXPos * (imageWdth / (double) imageWdthScaled) - 0.5 * Parameters::eyeAOIWdth);

                if (Parameters::eyeAOIXPos + Parameters::eyeAOIWdth >= imageWdth)
                {   Parameters::eyeAOIXPos = imageWdth - Parameters::eyeAOIWdth; }
                else if (Parameters::eyeAOIXPos < 0)
                {        Parameters::eyeAOIXPos = 0; }

                Parameters::eyeAOIXPosFraction = Parameters::eyeAOIXPos / (double) imageWdth;

                double mouseYPos = (event->y()) - imageScaledYOffset;

                Parameters::eyeAOIYPos = round(mouseYPos * (imageHght / (double) imageHghtScaled) - 0.5 * Parameters::eyeAOIHght);

                if (Parameters::eyeAOIYPos + Parameters::eyeAOIHght >= imageHght)
                {   Parameters::eyeAOIYPos = imageHght - Parameters::eyeAOIHght; }
                else if (Parameters::eyeAOIYPos < 0)
                {        Parameters::eyeAOIYPos = 0; }

                Parameters::eyeAOIYPosFraction = Parameters::eyeAOIYPos / (double) imageHght;

                setAOIEye(Parameters::eyeAOIXPos, Parameters::eyeAOIYPos, Parameters::eyeAOIWdth, Parameters::eyeAOIHght);
                setImage();
            }

            emit updateImage(-1);
        }
        else if (event->button() == Qt::RightButton)
        {
            { std::lock_guard<std::mutex> mainMutexLock(Parameters::mainMutex);

                double mouseXPos = (event->x()) - imageScaledXOffset;

                Parameters::beadAOIXPos = round(mouseXPos * (imageWdth / (double) imageWdthScaled) - 0.5 * Parameters::beadAOIWdth);

                if (Parameters::beadAOIXPos + Parameters::beadAOIWdth >= imageWdth)
                {   Parameters::beadAOIXPos = imageWdth - Parameters::beadAOIWdth; }
                else if (Parameters::beadAOIXPos < 0)
                {        Parameters::beadAOIXPos = 0; }

                Parameters::beadAOIXPosFraction = Parameters::beadAOIXPos / (double) imageWdth;

                double mouseYPos = (event->y()) - imageScaledYOffset;

                Parameters::beadAOIYPos = round(mouseYPos * (imageHght / (double) imageHghtScaled) - 0.5 * Parameters::beadAOIHght);

                if (Parameters::beadAOIYPos + Parameters::beadAOIHght >= imageHght)
                {   Parameters::beadAOIYPos = imageHght - Parameters::beadAOIHght; }
                else if (Parameters::beadAOIYPos < 0)
                {        Parameters::beadAOIYPos = 0; }

                Parameters::beadAOIYPosFraction = Parameters::beadAOIYPos / (double) imageHght;

                setAOIEye(Parameters::beadAOIXPos, Parameters::beadAOIYPos, Parameters::beadAOIWdth, Parameters::beadAOIHght);
                setImage();
            }

            emit updateImage(-1);
        }
    }
    else if (imageType == 2)
    {
        int imageScaledXOffset = 0.5 * (widgetWdth - imageWdthScaled);
        int imageScaledYOffset = 0.5 * (widgetHght - imageHghtScaled);

        if (event->button() == Qt::LeftButton)
        {
            double mouseXPos = (event->x()) - imageScaledXOffset;
            double mouseYPos = (event->y()) - imageScaledYOffset;

            double XPos = mouseXPos * (Parameters::eyeAOIWdth / (double) imageWdthScaled);
            double YPos = mouseYPos * (Parameters::eyeAOIHght / (double) imageHghtScaled);

            emit imageMouseClick(XPos, YPos);
        }
    }
}

