    /****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>
#include "scribblearea.h"
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
//#define LET_USER_DCLICK

int updateCntOS(int cntOS,int rad){
    /*
        Increase the brush size
    */
    if(cntOS%7!=0)
        return rad;
    if(rad<150)
        rad=rad+15;
    else
        rad = 300;
    return rad;
}


//! [0]
ScribbleArea:: ScribbleArea(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StaticContents);
    modified = false;
    scribbling = false;
    myPenWidth = 3;
    leftPenColor = Qt::green;
    rightPenColor = Qt::blue;
    saveCnt = 0;

    mIntSegm.param.height = NORMALHEIGHT;
    mIntSegm.param.width = NORMALWIDTH;
    mIntSegm.param.blockSize = 16;

    mIntSegm.sT->setWidget(this);

    currentWindow = new bool[NORMALHEIGHT*NORMALWIDTH];
    for(int i=0;i<NORMALWIDTH*NORMALHEIGHT;i++)
        currentWindow[i]=false;
    rad=20;
    cntOS = 0;
    lastOS = 0;
}
//! [0]

//! [1]
bool ScribbleArea::openImage(const QString &fileName)
//! [1] //! [2]
{
    this->reset();

    if (!loadedImage.load(fileName))
        return false;


    QSize newSize;
    newSize.setHeight(NORMALHEIGHT);
    newSize.setWidth(NORMALWIDTH);
    loadedImage=loadedImage.scaled(newSize,Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    mIntSegm.setImage(&loadedImage);
    mIntSegm.overSegment();
    image = loadedImage;
    origC = image;
    origG = image;
    /*
        Conversion to gray
    */
    QColor c;
    int Y;
    for(int w=0;w<NORMALWIDTH;w++)
        for(int h=0;h<NORMALHEIGHT;h++)
        {
            c=origC.pixel(w,h);
            Y = min(255,max(0,(  0.257*c.red()+0.504*c.green()+0.098*c.blue()+16)));
            Y = min(255,max(Y*0.75,0));
            origG.setPixel(w,h,QColor(Y,Y,Y).rgb());
         }

    image=origG;
    mIntSegm.drawOSBoundaryOnImage(&image);

    modified = false;
    update();

  //  char buffer[5];
  //  sprintf(buffer, "%d.jpg", saveCnt);
    //image.save(buffer, 0, -1);
    //saveCnt++;

    return true;
}
//! [2]

//! [3]
bool ScribbleArea::saveImage(const QString &fileName, const char *fileFormat)
//! [3] //! [4]
{
    QImage visibleImage = image;
    resizeImage(&visibleImage, size());

    if (visibleImage.save(fileName, fileFormat)) {
        modified = false;
        return true;
    } else {
        return false;
    }
}


void ScribbleArea::setPenWidth(int newWidth)
{
    myPenWidth = newWidth;
}

void ScribbleArea::clearImage()
{

    image.fill(qRgb(255, 255, 255));
    modified = true;
    update();
}

void ScribbleArea::mousePressEvent(QMouseEvent *event)
{

    static int contTouch=0;
    /*
        Update brush size
    */
    if(cntOS==0){
        cntOS++;
        lastOS=mIntSegm.SegmentID[NORMALWIDTH*event->pos().y()+event->pos().x()];
    }else{
        if(lastOS!=mIntSegm.SegmentID[NORMALWIDTH*event->pos().y()+event->pos().x()])
        {
            rad=updateCntOS(cntOS,rad);
            cntOS++;
            lastOS = mIntSegm.SegmentID[NORMALWIDTH*event->pos().y()+event->pos().x()];
        }

    }
    //Let user make brush size whole image by double clicking
#ifdef LET_USER_DCLICK
    if(mIntSegm.COLORING==mIntSegm.mSegmentationMethod)
        contTouch++;
#endif
    //Propagate the gesture to interactice segmentation class
    if (event->button() == Qt::LeftButton) {
        lastPoint = event->pos();
        scribbling = true;
        isLeft = true;
        mIntSegm.insertPoint(lastPoint.x(),lastPoint.y(),true);
    }

    if(event->button() == Qt::RightButton){
        lastPoint = event->pos();
        scribbling = true;
        isLeft = false;
        mIntSegm.insertPoint(lastPoint.x(),lastPoint.y(),false);
    }

    //Update the visible region according to brsuh size
    if(mIntSegm.mSegmentationMethod==mIntSegm.COLORING)
        for(int x=max(0,event->pos().x()-rad);x<min(event->pos().x()+rad,NORMALWIDTH);x++)
            for(int y=max(0,event->pos().y()-rad);y<min(event->pos().y()+rad,NORMALHEIGHT);y++)
                if(((x-lastPoint.x())*(x-lastPoint.x())+(y-lastPoint.y())*(y-lastPoint.y()))<rad*rad)
                    if((x<NORMALWIDTH)&&(y<NORMALHEIGHT))
                        currentWindow[NORMALWIDTH*y+x]=true;
#ifdef LET_USER_DCLICK
    if(contTouch>2)
        for(int x=0;x<NORMALWIDTH;x++)
            for(int y=0;y<NORMALHEIGHT;y++)
                    currentWindow[NORMALWIDTH*y+x]=true;
#endif
    //If just selecting over segments, simply draw result
    if(mIntSegm.mSegmentationMethod==mIntSegm.NOGCUT)
        drawResult();

}

void ScribbleArea::mouseMoveEvent(QMouseEvent *event)
{
    if (((event->buttons() & Qt::LeftButton) ||(event->buttons() & Qt::RightButton))&& scribbling){
        drawLineTo(event->pos());
        if(event->buttons() & Qt::LeftButton)
            mIntSegm.insertPoint(event->pos().x(),event->pos().y(),true);
        else
            mIntSegm.insertPoint(event->pos().x(),event->pos().y(),false);

        if(mIntSegm.mSegmentationMethod==mIntSegm.COLORING)
            for(int x=max(0,event->pos().x()-rad);x<min(event->pos().x()+rad,NORMALWIDTH);x++)
                for(int y=max(0,event->pos().y()-rad);y<min(event->pos().y()+rad,NORMALHEIGHT);y++)
                    if(((x-event->pos().x())*(x-event->pos().x())+(y-event->pos().y())*(y-event->pos().y()))<rad*rad)
                        if((x<NORMALWIDTH)&&(y<NORMALHEIGHT))
                            currentWindow[NORMALWIDTH*y+x]=true;

    }
    if(cntOS==0){
        lastOS=mIntSegm.SegmentID[NORMALWIDTH*event->pos().y()+event->pos().x()];
        cntOS++;
    }
    else{
        if(lastOS!=mIntSegm.SegmentID[NORMALWIDTH*event->pos().y()+event->pos().x()])
        {
            rad=updateCntOS(cntOS,rad);
            cntOS++;
            lastOS = mIntSegm.SegmentID[NORMALWIDTH*event->pos().y()+event->pos().x()];
        }

    }

    if(mIntSegm.mSegmentationMethod==mIntSegm.NOGCUT)
        drawResult();
}

void ScribbleArea::mouseReleaseEvent(QMouseEvent *event)
{

    if (((event->buttons() & Qt::LeftButton) ||(event->buttons() & Qt::RightButton))&& scribbling)
    {
        drawLineTo(event->pos());
        scribbling = false;
        if(event->buttons() & Qt::LeftButton)
            mIntSegm.insertPoint(event->pos().x(),event->pos().y(),true);
        else
            mIntSegm.insertPoint(event->pos().x(),event->pos().y(),false);

        if(mIntSegm.mSegmentationMethod==mIntSegm.COLORING)
            for(int x=max(0,event->pos().x()-rad);x<min(event->pos().x()+rad,NORMALWIDTH);x++)
                for(int y=max(0,event->pos().y()-rad);y<min(event->pos().y()+rad,NORMALHEIGHT);y++)
                    if(((x-event->pos().x())*(x-event->pos().x())+(y-event->pos().y())*(y-event->pos().y()))<rad*rad)
                        if((x<NORMALWIDTH)&&(y<NORMALHEIGHT))
                            currentWindow[NORMALWIDTH*y+x]=true;

    }
}

void ScribbleArea::paintEvent(QPaintEvent *event)
//! [13] //! [14]
{
    QPainter painter(this);
    QRect dirtyRect = event->rect();
    painter.drawImage(dirtyRect, image, dirtyRect);
}

void ScribbleArea::resizeEvent(QResizeEvent *event)
{
    /*
    if (width() > image.width() || height() > image.height()) {
        int newWidth = qMax(width() + 128, image.width());
        int newHeight = qMax(height() + 128, image.height());
        resizeImage(&image, QSize(newWidth, newHeight));
        update();
    }
    */
    QWidget::resizeEvent(event);
}
//! [16]

//! [17]
void ScribbleArea::drawLineTo(const QPoint &endPoint)
//! [17] //! [18]
{
    QPainter painter(&image);
    if(isLeft)
        painter.setPen(QPen(leftPenColor, myPenWidth, Qt::SolidLine, Qt::RoundCap,Qt::RoundJoin));
    else
        painter.setPen(QPen(rightPenColor, myPenWidth, Qt::SolidLine, Qt::RoundCap,Qt::RoundJoin));

    painter.drawLine(lastPoint, endPoint);
    modified = true;

    int rad2 = (myPenWidth / 2) + 2;
    update(QRect(lastPoint, endPoint).normalized()
                                     .adjusted(-rad2, -rad2, +rad2, +rad2));
    lastPoint = endPoint;
}
//! [18]

//! [19]
void ScribbleArea::resizeImage(QImage *image, const QSize &newSize)
//! [19] //! [20]
{
    if (image->size() == newSize)
        return;

    QImage newImage(newSize, QImage::Format_RGB32);
    newImage.fill(qRgb(255, 255, 255));
    QPainter painter(&newImage);
    painter.drawImage(QPoint(0, 0), *image);
    *image = newImage;
}
//! [20]

//! [21]
void ScribbleArea::print()
{
#ifndef QT_NO_PRINTER
    QPrinter printer(QPrinter::HighResolution);
 
    QPrintDialog *printDialog = new QPrintDialog(&printer, this);
//! [21] //! [22]
    if (printDialog->exec() == QDialog::Accepted) {
        QPainter painter(&printer);
        QRect rect = painter.viewport();
        QSize size = image.size();
        size.scale(rect.size(), Qt::KeepAspectRatio);
        painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter.setWindow(image.rect());
        painter.drawImage(0, 0, image);
    }
#endif // QT_NO_PRINTER
}

void ScribbleArea::setSegmentationMode(int type){
    if(type==0)
        mIntSegm.setSegmentationMethod(ISegmentation::BATCH);
    else if(type==1)
        mIntSegm.setSegmentationMethod(ISegmentation::COLORING);
    else
        mIntSegm.setSegmentationMethod(ISegmentation::NOGCUT);
}
//! [22]

void ScribbleArea::reset(){

    image = origG;
    mIntSegm.reset();
    for(int x=0;x<NORMALWIDTH;x++)
        for(int y=0;y<NORMALHEIGHT;y++)
                currentWindow[NORMALWIDTH*y+x]=false;
    rad = 20;
    cntOS = 0;
    lastOS = 0;
    update();
}

void ScribbleArea:: segmentViaCut(){
    mIntSegm.segmentBatch();
    for(int w=0;w<mIntSegm.param.width;w++)
        for(int h=0;h<mIntSegm.param.height;h++)
            if((mIntSegm.regionNode[mIntSegm.SegmentID[h*mIntSegm.param.width+w]].groupID)==1)
                image.setPixel(w,h,origC.pixel(w,h));
            else if((mIntSegm.regionNode[mIntSegm.SegmentID[h*mIntSegm.param.width+w]].groupID)==2)
                image.setPixel(w,h,origG.pixel(w,h));
    update();
}

void ScribbleArea:: drawResult(){
    for(int i=0;i<mIntSegm.queueFG.size();i++)
        mIntSegm.regionNode[mIntSegm.queueFG[i]].groupID =1;

    if(mIntSegm.mSegmentationMethod==mIntSegm.NOGCUT)
    {
        for(int w=0;w<mIntSegm.param.width;w++)
            for(int h=0;h<mIntSegm.param.height;h++)
                    if((mIntSegm.regionNode[mIntSegm.SegmentID[h*mIntSegm.param.width+w]].groupID)==1)
                        image.setPixel(w,h,origC.pixel(w,h));
                    else if((mIntSegm.regionNode[mIntSegm.SegmentID[h*mIntSegm.param.width+w]].groupID)==2)
                        image.setPixel(w,h,origG.pixel(w,h));
    }else
    {
        for(int w=0;w<mIntSegm.param.width;w++)
            for(int h=0;h<mIntSegm.param.height;h++)
                if(currentWindow[h*NORMALWIDTH+w])
                {
                    if((mIntSegm.regionNode[mIntSegm.SegmentID[h*mIntSegm.param.width+w]].groupID)==1)
                        image.setPixel(w,h,origC.pixel(w,h));
                    else if((mIntSegm.regionNode[mIntSegm.SegmentID[h*mIntSegm.param.width+w]].groupID)==2)
                        image.setPixel(w,h,origG.pixel(w,h));
                }else{
                        mIntSegm.regionNode[mIntSegm.SegmentID[h*mIntSegm.param.width+w]].groupID=2;
                        image.setPixel(w,h,origG.pixel(w,h));
                }
    }
         update();
}

void ScribbleArea::computeDepth(){
    mIntSegm.computeDepth();
}

void ScribbleArea::changeView(int idd){
    mIntSegm.changeView(&image,idd);
    update();
    char buffer[5];
    sprintf(buffer, "%d.jpg", saveCnt);
    image.save("abc.jpg", 0, -1);
    saveCnt++;
}

void ScribbleArea::changeDepthMode(int dMode)
{
    mIntSegm.changeDepthMode(dMode);
    mIntSegm.changeView(&image,2);
    update();
    char buffer[5];
    sprintf(buffer, "%d.jpg", saveCnt);
    //image.save(buffer, 0, -1);
    saveCnt++;
}

void ScribbleArea::changeAlphaMatte(int matteSize)
{
    mIntSegm.changeAlphaMatte(matteSize);
    mIntSegm.changeView(&image,2);
    update();
    char buffer[5];
    sprintf(buffer, "%d.jpg", saveCnt);
    //image.save(buffer, 0, -1);
    saveCnt++;
}

void ScribbleArea::changeFGEnlargement(int mode)
{
    mIntSegm.changeFGEnlargementMode(mode);
    mIntSegm.changeView(&image,2);
    update();
    char buffer[5];
    sprintf(buffer, "%d.jpg", saveCnt);
    //image.save(buffer, 0, -1);
    saveCnt++;
}
void ScribbleArea::changeBlur(int mode)
{
    mIntSegm.changeBlurMode(mode);
    mIntSegm.changeView(&image,2);
    update();
    char buffer[5];
    sprintf(buffer, "%d.jpg", saveCnt);
    //image.save(buffer, 0, -1);
    saveCnt++;
}

void ScribbleArea::enableSPMerge(int md){
//2 checked 0 unchecked
    if(md!=0)
        this->mIntSegm.isMerge = true;
    else
        this->mIntSegm.isMerge = false;
}
