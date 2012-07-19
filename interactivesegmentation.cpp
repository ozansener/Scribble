#include <QtGui>
#include "interactivesegmentation.h"
#include "SLIC.h"
//#include <fstream>
#include <iostream>
#include <QTextStream>
#include <QFile>
#include <QImage>
#include <vector>

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define absolut(a) (((a) > (0)) ? (a) : (-1*(a)))

#define HOR_TRANS 1 // horizontal translation
#define FOC_LENGTH 12 // focal length
#define DEFAULT_SCALE 1.1 // magnetization scale

ISegmentation::ISegmentation()
{
    isMerge = false;
    isFirstImage = true;
    reset();
    sT = new SegmentationThread;
    sT->setQueues(&queueBG,&queueFG);
    this->mSegmentationMethod = BATCH;

}

void ISegmentation::setSegmentationMethod(METHOD type){
    this->mSegmentationMethod = type;
    if(type==this->COLORING)
          sT->start(QThread::NormalPriority);
}

void ISegmentation::reset(){
    if(mSegmentationMethod==COLORING)
    {
        sT->reset();
        sT->wait(100);
    }
    queueBG.clear();
    queueFG.clear();
    gCut.reset();

}

int ISegmentation::getMinDistSP(int id)
{

    float capp;

    float minDist;
    int minID;
    minID = 0;
    capp = (regionNode[id].meanR -  regionNode[regionNode[id].neighborID[0]].meanR)*(regionNode[id].meanR - regionNode[regionNode[id].neighborID[0]].meanR);
    capp += (regionNode[id].meanG - regionNode[regionNode[id].neighborID[0]].meanG)*(regionNode[id].meanG - regionNode[regionNode[id].neighborID[0]].meanG);
    capp += (regionNode[id].meanB - regionNode[regionNode[id].neighborID[0]].meanB)*(regionNode[id].meanB - regionNode[regionNode[id].neighborID[0]].meanB);
    minDist = capp;

    for(int i=1;i<regionNode[id].neighborNum;i++)
    {
        capp = (regionNode[id].meanR -  regionNode[regionNode[id].neighborID[i]].meanR)*(regionNode[id].meanR - regionNode[regionNode[id].neighborID[i]].meanR);
        capp += (regionNode[id].meanG - regionNode[regionNode[id].neighborID[i]].meanG)*(regionNode[id].meanG - regionNode[regionNode[id].neighborID[i]].meanG);
        capp += (regionNode[id].meanB - regionNode[regionNode[id].neighborID[i]].meanB)*(regionNode[id].meanB - regionNode[regionNode[id].neighborID[i]].meanB);
        if(capp<minDist)
        {
            minDist=capp;
            minID = regionNode[id].neighborID[i];
        }

    }
    return minID;
}
void ISegmentation::mergeSP(int id)
{
    if(regionNode[id].neighborNum==0)
        return;
    int curID;
    vector<int> stackSP;
    stackSP.push_back(id);
    while(stackSP.size()!=0)
    {
        curID = stackSP.back();
        stackSP.pop_back();
        for(int i=0;i<regionNode[curID].neighborNum;i++)
            if(regionNode[regionNode[curID].neighborID[i]].neighborNum!=0)
                if((getMinDistSP(curID)==regionNode[curID].neighborID[i])&&(curID==getMinDistSP(regionNode[curID].neighborID[i])))
                    if(regionNode[regionNode[curID].neighborID[i]].groupID!=1)
                    {
                        regionNode[regionNode[curID].neighborID[i]].groupID=1;
                        stackSP.push_back(regionNode[curID].neighborID[i]);
                    }
    }

}

void ISegmentation::insertPoint(int x,int y,bool isFG){
    if((x>=param.width)||(x<0)||(y>=param.height)||(y<0))\
        return;
    if(isFG){
        queueFG.push_back(SegmentID[y*param.width+x]);
        if(mSegmentationMethod==BATCH)//If coloring
            gCut.add2FG(SegmentID[y*param.width+x]);
        if(mSegmentationMethod==NOGCUT)
        {
            regionNode[SegmentID[y*param.width+x]].groupID = 1;
            if(isMerge)
                mergeSP(SegmentID[y*param.width+x]);

        }
    }else{
        queueBG.push_back(SegmentID[y*param.width+x]);
        if(mSegmentationMethod==BATCH)
            gCut.add2BG(SegmentID[y*param.width+x]);
        if(mSegmentationMethod==NOGCUT)
        {
            if(regionNode[SegmentID[y*param.width+x]].groupID == 1)
                regionNode[SegmentID[y*param.width+x]].groupID = 2;
            for(int l=0;l<queueFG.size();l++)
                if(queueBG[l]==SegmentID[y*param.width+x]){
                    queueBG.erase(queueBG.begin()+l);
                    break;
                }

        }
    }
}

void ISegmentation::setImage(QImage* im){
    brushMap = new bool[param.width*param.height];
    inputImage = *im;

    depthMap = QImage(inputImage.size(), QImage::Format_RGB32);
    leftView = QImage(inputImage.size(), QImage::Format_RGB32);
    rightView = QImage(inputImage.size(), QImage::Format_RGB32);
    anaglyphImage = QImage(inputImage.size(), QImage::Format_RGB32);
    onlyForeground = QImage(inputImage.size(), QImage::Format_RGB32);
    onlyBackground = QImage(inputImage.size(), QImage::Format_RGB32);
    enlargedForeground = QImage(inputImage.size(), QImage::Format_RGB32);
    filledBackground = QImage(inputImage.size(), QImage::Format_RGB32);
    alphaBlend = QImage(inputImage.size(), QImage::Format_RGB32);
    leftAlpha = QImage(inputImage.size(), QImage::Format_RGB32);
    leftFg = QImage(inputImage.size(), QImage::Format_RGB32);
    leftBg = QImage(inputImage.size(), QImage::Format_RGB32);
    FGLocation = 0;
    BGLocation = 0;
    matteWidth = 5;
    FGEnlargementMode = 0;
    blurMode = 0;
}

void ISegmentation::overSegment(){
    if(isFirstImage)
    {
        initOverSegment();
        isFirstImage=false;
    }
        initOSImage();

    int width,height;
    width = param.width;
    height = param.height;
    QColor c;
    unsigned int* ubuff = new unsigned int[width*height];
    int r,g,b;
    for(int i=0;i<height;i++)
        for(int j=0;j<width;j++)
        {
            c = inputImage.pixel(j,i);
            r=c.red();
            g=c.green();
            b=c.blue();
            ubuff[i*width+j]= (r<<16) + (g<<8) + (b);
        }
    int blockSize=param.blockSize;
    int numlabels = 0;
    int m_spcount = 1000 ;
    int m_compactness = 20;
    SLIC slic;
    slic.DoSuperpixelSegmentation_ForGivenK(ubuff, width, height, SegmentID, numlabels, m_spcount, m_compactness);

    int max =0;
    for(int j=0;j<width*height;j++)
        if(SegmentID[j]>max)
            max=SegmentID[j];

    param.segmentNumber = max+1;
    createGraphStructure();
    gCut.setSegmentNumber(param.segmentNumber);
    gCut.setImage(&inputImage);
    gCut.setRegionNode(&regionNode);
    gCut.setSegmentID(SegmentID);
    gCut.width = param.width;
    gCut.height = param.height;
    sT->setGCUT(&gCut);
    for(int i=0;i<param.segmentNumber;i++)
        regionNode[i].groupID = 2;


}

void ISegmentation::initOverSegment(){

    int blockSize=param.blockSize;
    int height=param.height, width=param.width;
    int segmentNumber=height*width/(blockSize*blockSize);
    param.segmentNumber = segmentNumber;
    //memory allocation for the variables
    regionNode = new RegionNode[segmentNumber];
    SegmentID = (int *)   calloc(height* width, sizeof(int));


}

void ISegmentation::initOSImage(){

}

void ISegmentation::drawOSBoundaryOnImage(QImage* im){

    int width = param.width;
    int height = param.height;
    for (int h=1; h<height-1; h++)
    {
        for (int w=1; w<width-1; w++)
        {
            if(!((SegmentID[h*width+w]==SegmentID[h*width+w+1])&&(SegmentID[h*width+w]==SegmentID[(h+1)*width+w])))
                im->setPixel(w,h,0x80800000);
        }
    }
}

void ISegmentation::segmentBatch(){

    gCut.runGrabCut();
}

void ISegmentation::createGraphStructure(){
    static int fCnt = 0;
    for(int i=0;i<param.segmentNumber;i++){
        regionNode[i].neighborNum=0;
        regionNode[i].groupID = 0;
        regionNode[i].maxX = 0;
        regionNode[i].maxY = 0;
        regionNode[i].minX = param.width;
        regionNode[i].minY = param.height;
        regionNode[i].segmentSize = 0;
        regionNode[i].meanB=0;
        regionNode[i].meanG=0;
        regionNode[i].meanR=0;
        for(int j=0;j<10;j++)
        {
            regionNode[i].boundSize[j]=0;
            regionNode[i].neighborDistance[j]=0;
        }
    }
    int* numberOfNeighBoors = new int[param.segmentNumber*param.segmentNumber];
    memset(numberOfNeighBoors,0,param.segmentNumber*param.segmentNumber*sizeof(int));
    float * neighbDist = new float[param.segmentNumber*param.segmentNumber];
    memset(neighbDist,0,sizeof(float)*param.segmentNumber*param.segmentNumber);

    for(int w=0;w<param.width-1;w++)
        for(int h=0;h<param.height-1;h++)
        {
            numberOfNeighBoors[SegmentID[h*param.width+w]*param.segmentNumber+SegmentID[h*param.width+w+1]]++;
            numberOfNeighBoors[SegmentID[h*param.width+w]*param.segmentNumber+SegmentID[(h+1)*param.width+w]]++;
        }

    QColor c;
    for(int w=0;w<param.width;w++)
        for(int h=0;h<param.height;h++)
        {
            regionNode[SegmentID[h*param.width+w]].segmentSize++;
            c = inputImage.pixel(w,h);
            regionNode[SegmentID[h*param.width+w]].meanB+=c.blue();
            regionNode[SegmentID[h*param.width+w]].meanG+=c.green();
            regionNode[SegmentID[h*param.width+w]].meanR+=c.red();


            if(w>regionNode[SegmentID[h*param.width+w]].maxX)
                    regionNode[SegmentID[h*param.width+w]].maxX = w;
            if(h>regionNode[SegmentID[h*param.width+w]].maxY)
                    regionNode[SegmentID[h*param.width+w]].maxY = h;
            if(w<regionNode[SegmentID[h*param.width+w]].minX)
                regionNode[SegmentID[h*param.width+w]].minX = w;
            if(h<regionNode[SegmentID[h*param.width+w]].minY)
                regionNode[SegmentID[h*param.width+w]].minY = h;
        }
    for(int i=0;i<param.segmentNumber;i++)
        for(int j=0;j<param.segmentNumber;j++)
        {
            if(i==j)
                continue;
            if((numberOfNeighBoors[i*param.segmentNumber+j]!=0)||(numberOfNeighBoors[j*param.segmentNumber+i]!=0))
            {
//                if(regionNode[i].neighborNum<10)
//                {
//                    regionNode[i].neighborID[regionNode[i].neighborNum]=j;
//                    regionNode[i].neighborNum++;
//                }
                if(regionNode[j].neighborNum<10)
                {
                    regionNode[j].neighborID[regionNode[j].neighborNum]=i;
                    regionNode[j].neighborNum++;
                }
            }
        }


    for(int i=0;i<param.segmentNumber;i++)
    {
        regionNode[i].meanB = regionNode[i].meanB/regionNode[i].segmentSize;
        regionNode[i].meanR = regionNode[i].meanR/regionNode[i].segmentSize;
        regionNode[i].meanG = regionNode[i].meanG/regionNode[i].segmentSize;
    }
    int r,g,b;
    int binNum=16;
    int binDiv = 256/binNum;
    int binStep = binNum*binNum*binNum;
    int *histData = new int[param.segmentNumber*binStep];
    memset(histData,0,sizeof(int)*binStep);
    for(int i=0;i<param.width;i++)
        for(int j=0;j<param.height;j++)
        {
            c=inputImage.pixel(i,j);
            histData[SegmentID[j*param.width+i]*binStep + ((int)(c.red()/binDiv))*binNum*binNum + ((int)(c.green()/binDiv))*binNum + ((int)(c.blue()/binDiv))]++;
        }
    for(int i=0;i<param.segmentNumber;i++)
        for(int j=0;j<param.segmentNumber;j++)
            if(i!=j)
              if((numberOfNeighBoors[i*param.segmentNumber+j]!=0)||(numberOfNeighBoors[j*param.segmentNumber+i]!=0))
              {
                  for(int k=0;k<binStep;k++)
                  {
                      neighbDist[param.segmentNumber*i+j]+=sqrt(((float)(histData[i*binStep+k]*histData[j*param.segmentNumber+k]))/((float)(regionNode[i].segmentSize*regionNode[j].segmentSize)));
                     // neighbDist[param.segmentNumber*j+i]+=sqrt(((float)(histData[i*binStep+k]*histData[j*param.segmentNumber+k]))/((float)(regionNode[i].segmentSize*regionNode[j].segmentSize)));
                  }
              }
    for(int i=0;i<param.segmentNumber;i++)
    {
        for(int j=0;j<regionNode[i].neighborNum;j++)
        {
            regionNode[i].neighborDistance[j]=1/(neighbDist[param.segmentNumber*i+regionNode[i].neighborID[j]]+0.0001);
        }
    }
    //Find the boundary distance
    /*
    memset(numberOfNeighBoors,0,param.segmentNumber*param.segmentNumber*sizeof(int));
    float distTemp;
    QColor c2;
    for(int w=0;w<param.width-1;w++)
        for(int h=0;h<param.height-1;h++)
        {
            if(SegmentID[h*param.width+w]!=SegmentID[h*param.width+w+1])
            {
                c=inputImage.pixel(w,h);
                c2=inputImage.pixel(w+1,h);
                numberOfNeighBoors[param.segmentNumber*SegmentID[h*param.width+w]+SegmentID[h*param.width+w+1]]++;
                numberOfNeighBoors[param.segmentNumber*SegmentID[h*param.width+w+1]+SegmentID[h*param.width+w]]++;
                distTemp=0;
                distTemp+=abs(c.red()-c2.red());
                distTemp+=abs(c.blue()-c2.blue());
                distTemp+=abs(c.green()-c2.green());
                neighbDist[param.segmentNumber*SegmentID[h*param.width+w]+SegmentID[h*param.width+w+1]]+=distTemp/255.0;
                neighbDist[param.segmentNumber*SegmentID[h*param.width+w+1]+SegmentID[h*param.width+w]]+=distTemp/255.0;
            }
            if(SegmentID[h*param.width+w]!=SegmentID[(h+1)*param.width+w])
            {
                c=inputImage.pixel(w,h);
                c2=inputImage.pixel(w,h+1);
                numberOfNeighBoors[param.segmentNumber*SegmentID[h*param.width+w]+SegmentID[(h+1)*param.width+w]]++;
                numberOfNeighBoors[param.segmentNumber*SegmentID[(h+1)*param.width+w]+SegmentID[h*param.width+w]]++;
                distTemp=0;
                distTemp+=abs(c.red()-c2.red());
                distTemp+=abs(c.blue()-c2.blue());
                distTemp+=abs(c.green()-c2.green());
                neighbDist[param.segmentNumber*SegmentID[h*param.width+w]+SegmentID[(h+1)*param.width+w]]+=distTemp/255.0;
                neighbDist[param.segmentNumber*SegmentID[(h+1)*param.width+w]+SegmentID[h*param.width+w]]+=distTemp/255.0;
            }
            if(SegmentID[h*param.width+w]!=SegmentID[(h+1)*param.width+w+1])
            {
                c=inputImage.pixel(w,h);
                c2=inputImage.pixel(w+1,h+1);
                numberOfNeighBoors[param.segmentNumber*SegmentID[h*param.width+w]+SegmentID[(h+1)*param.width+w+1]]++;
                numberOfNeighBoors[param.segmentNumber*SegmentID[(h+1)*param.width+w+1]+SegmentID[h*param.width+w]]++;
                distTemp=0;
                distTemp+=abs(c.red()-c2.red());
                distTemp+=abs(c.blue()-c2.blue());
                distTemp+=abs(c.green()-c2.green());
                neighbDist[param.segmentNumber*SegmentID[h*param.width+w]+SegmentID[(h+1)*param.width+w+1]]+=distTemp/255.0;
                neighbDist[param.segmentNumber*SegmentID[(h+1)*param.width+w+1]+SegmentID[h*param.width+w]]+=distTemp/255.0;
            }
        }
    float r1,r2,g1,g2,b1,b2,h1,h2,s1,s2,v1,v2;
    float mx1,mn1,mx2,mn2,delta1,delta2;
    for(int i=0;i<param.segmentNumber;i++)
    {



        r1 = regionNode[i].meanR;
        g1 = regionNode[i].meanG;
        b1 = regionNode[i].meanB;
        mx1 = max(r1,max(g1,b1));
        mn1 = min(r1,min(g1,b1));
        delta1 = mx1-mn1;
        v1 = mx1;
        if(delta1==0)
        {
            h1=0;
        }else{
            if(r1==mx1)
            {
                h1 = (g1 - b1)/delta1;
            }else if(g1==mx1)
            {
                h1 = 2 + (b1-r1)/delta1;
            }else{
                h1 = 4 + (r1-g1)/delta1;
            }
        }
        if(delta1==0)
        {
            s1=0;
        }else{
            s1=delta1/mx1;
        }



        for(int j=0;j<regionNode[i].neighborNum;j++)
        {
            r2 = regionNode[regionNode[i].neighborID[j]].meanR;
            g2 = regionNode[regionNode[i].neighborID[j]].meanG;
            b2 = regionNode[regionNode[i].neighborID[j]].meanB;

            mx2 = max(r2,max(g2,b2));
            mn2 = min(r2,min(g2,b2));
            delta2 = mx2-mn2;
            v2 = mx2;
            if(delta2==0)
            {
                h2=0;
            }else{
                if(r2==mx2)
                {
                    h2 = (g2 - b2)/delta2;
                }else if(g2==mx2)
                {
                    h2 = 2 + (b2-r2)/delta2;
                }else{
                    h2 = 4 + (r2-g2)/delta2;
                }
            }
            if(delta2==0)
            {
                s2=0;
            }else{
                s2=delta2/mx2;
            }



            regionNode[i].boundSize[j]=numberOfNeighBoors[param.segmentNumber*i+regionNode[i].neighborID[j]];
   //         regionNode[i].neighborDistance[j]=neighbDist[param.segmentNumber*i+regionNode[i].neighborID[j]]/(regionNode[i].boundSize[j]);
 //          regionNode[i].neighborDistance[j]= abs(regionNode[i].meanR-regionNode[regionNode[i].neighborID[j]].meanR)+abs(regionNode[i].meanG-regionNode[regionNode[i].neighborID[j]].meanG)+abs(regionNode[i].meanB-regionNode[regionNode[i].neighborID[j]].meanB);
   //        regionNode[i].neighborDistance[j]= abs(-regionNode[regionNode[i].neighborID[j]].meanR)*abs(regionNode[i].meanR-regionNode[regionNode[i].neighborID[j]].meanR)+abs(regionNode[i].meanG-regionNode[regionNode[i].neighborID[j]].meanG)*abs(regionNode[i].meanG-regionNode[regionNode[i].neighborID[j]].meanG)+abs(regionNode[i].meanB-regionNode[regionNode[i].neighborID[j]].meanB)*abs(regionNode[i].meanB-regionNode[regionNode[i].neighborID[j]].meanB);
      //      regionNode[i].neighborDistance[j] = sqrt((double)((h1-h2)*(h1-h2)+(s1-s2)*(s1-s2)+(v1-v2)*(v1-v2)));
            regionNode[i].neighborDistance[j] =sqrt((double)((r1-r2)*(r1-r2)+(g1-g2)*(g1-g2)+(b1-b2)*(b1-b2)));
            //   if(isnan(abs(h1-h2)+abs(s1-s2)+abs(v1-v2)))
           //     std::cout<<"NAN:"<<h1<<" "<<h2<<" "<<s1<<" "<<s2<<" "<<v1<<" "<<v2<<std::endl;
        }
    }

    */

    delete [] neighbDist;
    delete [] numberOfNeighBoors;


}

void ISegmentation::changeView(QImage* im,int which)
{
    /*
    0 Input
    1 Depth
    2 Anaglyph
    3 Left
    4 Right
    5 Only FG
    6 Enlarged FG
    7 Filled BG
    8 Alpha Matte
    9 Segmentation Result
    */
/*
    QSize newSize;
    newSize.setHeight(1080);
    newSize.setWidth(1920);


    QImage left;
    QImage right;
    left = leftView;
    right = rightView;
    left = left.scaled(newSize,Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    right = right.scaled(newSize,Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    newSize.setHeight(1080);
    newSize.setWidth(3840);
    QImage result=QImage(newSize,QImage::Format_RGB32);
    int a = result.width();
    int b= result.height();
    QPainter painter;


    for(int x=0;x<1920;x++)
        for(int y=0;y<1080;y++)
        {
            result.setPixel(x,y,left.pixel(x,y));
            result.setPixel(x+1920,y,right.pixel(x,y));
        }
    painter.begin(&result);
    painter.setFont(QFont("Ubuntu",64));
    painter.setPen(QColor(255,255,255));
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawText(100,100,QString("12"));
    painter.end();

    painter.begin(&result);
    painter.setFont(QFont("Ubuntu",64));
    painter.setPen(QColor(255,255,255));
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawText(100+1920,100,QString("12"));
    painter.end();

    result.save("Result3D.jpg");
*/
    for(int w=0;w<param.width;w++)
        for(int h=0;h<param.height;h++)
            switch(which){
            case 0:
                im->setPixel(w,h, inputImage.pixel(w, h));
                break;
            case 1:
                im->setPixel(w,h,depthMap.pixel(w, h));
                break;
            case 2:
                im->setPixel(w,h,anaglyphImage.pixel(w, h));
                break;
            case 3:
                im->setPixel(w,h,leftView.pixel(w, h));
                break;
            case 4:
                im->setPixel(w,h,rightView.pixel(w,h));
                break;
            case 5:
                im->setPixel(w,h,onlyForeground.pixel(w,h));
                break;
            case 6:
                im->setPixel(w,h,enlargedForeground.pixel(w,h));
                break;
            case 7:
                im->setPixel(w,h,filledBackground.pixel(w,h));
                break;
            case 8:
                im->setPixel(w,h,alphaBlend.pixel(w,h));
                break;
            case 9:
                if((regionNode[SegmentID[h*param.width+w]].groupID)==1)
                    im->setPixel(w,h,inputImage.pixel(w,h));
                else
                    im->setPixel(w,h,qRgb(255,255,255));
                break;
            }

}

void ISegmentation::computeDepth()
{
 // yagiz
    int wMax = depthMap.width();
    int hMax = depthMap.height();
    double dTemp;

    depthMap.fill(0);
    rightView.fill(0);
    leftView.fill(0);
    anaglyphImage.fill(0);
    onlyForeground.fill(0);
    onlyBackground.fill(0);
    enlargedForeground.fill(0);
    filledBackground.fill(0);
    alphaBlend.fill(0);
    leftFg.fill(0);
    leftBg.fill(0);
    leftAlpha.fill(0);

    groupIdRight = new int[wMax * hMax];
/*
    QSize newSize;
    newSize.setHeight(hMax);
    newSize.setWidth(wMax);

    QImage imSegRes;
    imSegRes.load("segMap2.jpg");
    imSegRes = imSegRes.scaled(newSize);


    QColor c;
    for(int y=0;y<hMax;y++)
        for(int x=0;x<wMax;x++)
        {
            c = imSegRes.pixel(x,y);
            if((c.red()<10)&&(c.green()<10)&&(c.blue()<10))
                groupIdRight[y*wMax+x]=2;
            else
                groupIdRight[y*wMax+x]=1;

        }
*/

    for ( int i = 0; i < wMax * hMax; i++)
        groupIdRight[i] = regionNode[ SegmentID[i] ].groupID;

    modifyFGMap();

    for ( int w = 0; w < wMax; w++)
        for ( int h = 0; h < hMax; h++)
            switch ( groupIdRight[ h * wMax + w ] )
            {
            case 1:
                depthMap.setPixel(w, h, qRgb(1, 1, 1));
                break;
            case 2:
                dTemp = 255 * ( 1 - (double) h / (double) hMax ) + 1;
                //depthMap.setPixel(w, h, qRgb(dTemp, dTemp, dTemp));
                depthMap.setPixel(w, h, qRgb(255, 255, 255));
                break;
            }

    int gaussRadius = 7;
    int tempSum;

    QImage depthMapTemp = depthMap;

    for ( int h = 0; h < hMax; h++ )
        for ( int w = gaussRadius; w < wMax-gaussRadius; w++ )
        {
            tempSum = 0;

            for ( int i = -gaussRadius; i <= gaussRadius; i++ )
                    tempSum += qRed(depthMapTemp.pixel(w+i, h));

            tempSum = tempSum / (2 * gaussRadius + 1);

            depthMap.setPixel(w, h, qRgb(tempSum, tempSum, tempSum));

        }


    if ( FGEnlargementMode == 0 )
    {
        fillBackground();
        if (blurMode != 0)
        {
            blurBG();
            blurFG();
        }
        computeAlphaMatteMap();
        depthBasedRendering();
        createAnaglyphImage();
    }
    else
    {
        enlargeForeground(DEFAULT_SCALE);
        fillBackground2();
        if (blurMode != 0)
        {
            blurBG();
            blurFG();
        }
        computeAlphaMatteMap2();
        depthBasedRendering2();
        createAnaglyphImage();
    }

}

void ISegmentation::depthBasedRendering()
{
    int wMax = depthMap.width();
    int hMax = depthMap.height();
    int wNew;
    int pixDepth;

    leftView.fill(0);
    rightView.fill(0);

    // Generate Left Image

    groupIdLeft = new int[wMax * hMax];

    for ( int i = 0; i < wMax*hMax; i++)
        groupIdLeft[i] = 0;


    for ( int w = 0; w < wMax; w++)
        for ( int h = 0; h < hMax; h++)
        {
            pixDepth = 1;
            wNew = w + FOC_LENGTH * HOR_TRANS / pixDepth - FGLocation * FOC_LENGTH * HOR_TRANS;
            if ( (wNew >= 0) && (wNew < wMax) )
            {
                leftFg.setPixel(wNew, h, onlyForeground.pixel(w, h));
                leftAlpha.setPixel(wNew, h, alphaBlend.pixel(w, h));
                if ( groupIdRight[h * wMax + w] == 1)
                    groupIdLeft[ h * wMax + wNew ] = 1;
            }
        }

    for ( int w = 0; w < wMax; w++)
        for ( int h = 0; h < hMax; h++)
        {
            pixDepth = 255 * ( 1 - (double) h / (double) hMax ) + 1;
            wNew = w + FOC_LENGTH * HOR_TRANS / pixDepth - BGLocation * FOC_LENGTH * HOR_TRANS;
            if ( (wNew >= 0) && (wNew < wMax) )
            {
                leftBg.setPixel(wNew, h, filledBackground.pixel(w, h));
            }
        }

    double alphaCoeff;
    int tempR, tempG, tempB;

    for ( int w = 0; w < wMax; w++)
        for ( int h = 0; h < hMax; h++)
        {
            if ( groupIdLeft[h * wMax + w] != 1 )
            {
                leftView.setPixel( w, h, leftBg.pixel(w,h));
                continue;
            }
            alphaCoeff = (double)qRed(leftAlpha.pixel(w,h)) / 255;
            tempR = alphaCoeff * (double)qRed(leftFg.pixel(w,h)) +
                    ( 1 - alphaCoeff ) * (double)qRed(leftBg.pixel(w,h));
            tempG = alphaCoeff * (double)qGreen(leftFg.pixel(w,h)) +
                    ( 1 - alphaCoeff ) * (double)qGreen(leftBg.pixel(w,h));
            tempB = alphaCoeff * (double)qBlue(leftFg.pixel(w,h)) +
                    ( 1 - alphaCoeff ) * (double)qBlue(leftBg.pixel(w,h));
            leftView.setPixel( w, h, qRgb(tempR, tempG, tempB));
        }


    // Generate Right Image

   // rightView = inputImage;

    for ( int w = 0; w < wMax; w++)
        for ( int h = 0; h < hMax; h++)
        {
            if ( groupIdRight[h * wMax + w] != 1 )
                rightView.setPixel( w, h, filledBackground.pixel(w,h));
            else
                rightView.setPixel( w, h, onlyForeground.pixel(w,h));
        }



}

void ISegmentation::fillBackground()
{
    int hMax = depthMap.height();
    int wMax = depthMap.width();

    for ( int w = 0; w < wMax; w++)
        for ( int h = 0; h < hMax; h++)
            switch ( groupIdRight[ h * wMax + w ] )
            {
            case 1:
                onlyForeground.setPixel(w, h, inputImage.pixel(w, h));
                onlyBackground.setPixel(w, h,qRgb(255,255,255));
                break;
            case 2:
                onlyForeground.setPixel(w, h, qRgb(255,255,255));
                onlyBackground.setPixel(w, h, inputImage.pixel(w, h));
                break;
            }

    for ( int h = 0; h < hMax; h++ )
        for ( int w = 0; w < wMax; w++ )
            filledBackground.setPixel(w, h, onlyBackground.pixel(w, h));

    if ( groupIdRight[0] != 2 )
        filledBackground.setPixel(0, 0, qRgb(100,100,100));

    for ( int h = 1; h < hMax; h++ )
        if ( groupIdRight[h * wMax ] != 2 )
            filledBackground.setPixel(0, h, filledBackground.pixel(0, h-1));

    for ( int h = 0; h < hMax; h++ )
        for ( int w = 1; w < wMax; w++ )
            if ( groupIdRight[h * wMax + w] == 1 )
                filledBackground.setPixel(w, h, filledBackground.pixel(w-1, h));

}

void ISegmentation::createAnaglyphImage()
{
    int wMax = depthMap.width();
    int hMax = depthMap.height();
    int redCh;
    int blueCh;

    for ( int w = 0; w < wMax; w++)
        for ( int h = 0; h < hMax; h++)
        {
            blueCh = (qRed(rightView.pixel(w, h)) + qGreen(rightView.pixel(w, h)) + qBlue(rightView.pixel(w, h))) / 3;
            redCh = (qRed(leftView.pixel(w, h)) + qGreen(leftView.pixel(w, h)) + qBlue(leftView.pixel(w, h))) / 3;
            anaglyphImage.setPixel(w, h, qRgb( redCh, 0, blueCh));
        }

}

void ISegmentation::enlargeForeground(double scale = DEFAULT_SCALE)
{
    int wMax = depthMap.width();
    int hMax = depthMap.height();

    double* points;

    int fgCounter = 0;
    int fgSumH = 0;
    int fgSumW = 0;
    QPoint topLeft = QPoint(wMax,hMax);
    QPoint bottomRight = QPoint(0,0);

    // x -> w, y -> h

    // Extract foreground and calculate its center of mass
    for ( int w = 0; w < wMax; w++)
        for ( int h = 0; h < hMax; h++)
            switch ( groupIdRight[ h * wMax + w ] )
            {
            case 1:
                onlyForeground.setPixel(w, h, inputImage.pixel(w, h));
                onlyBackground.setPixel(w, h, 0);
                fgSumH += h;
                fgSumW += w;
                fgCounter++;
                if (w < topLeft.x())
                    topLeft.setX(w);
                if (w > bottomRight.x())
                    bottomRight.setX(w);
                if (h < topLeft.y())
                    topLeft.setY(h);
                if (h > bottomRight.y())
                    bottomRight.setY(h);
                break;
            case 2:
                onlyForeground.setPixel(w, h, 0);
                onlyBackground.setPixel(w, h, inputImage.pixel(w, h));
                break;
            }

    QPoint centerOfMass = QPoint(fgSumW / fgCounter, fgSumH / fgCounter);

    double stepSize = 1 / scale;

    int pointCountH = ( bottomRight.y() - topLeft.y() ) / stepSize + 150;
    int pointCountW = ( bottomRight.x() - topLeft.x() ) / stepSize + 150;

    // Make sure odd number of new point, in order to place CoM in the middle
    if ( !( pointCountH % 2 ) )
        pointCountH++;
    if ( !( pointCountW % 2 ) )
        pointCountW++;


    // Create a 3D vector containing pseudo coordinates

    points = new double[pointCountH * pointCountW * 2];

    for ( int i = 0; i < pointCountH * pointCountW * 2; i++)
        points[i] = 0;

    int midPtH = (pointCountH - 1) / 2;
    int midPtW = (pointCountW - 1) / 2;

  //  points[midPtH][midPtW][1] = centerOfMass.x();
  //  points[midPtH][midPtW][2] = centerOfMass.y();

    for (int  h = 0; h < pointCountH; h++ )
        for ( int w = 0; w < pointCountW; w++ )
        {
            points[h * pointCountW + w] = centerOfMass.x() - (midPtW - w) * stepSize;
            points[h * pointCountW + w + pointCountH * pointCountW] = centerOfMass.y() - (midPtH - h) * stepSize;

            if ( points[h * pointCountW + w] < 0 ||
                 points[h * pointCountW + w] > wMax ||
                 points[h * pointCountW + w + pointCountH * pointCountW] < 0 ||
                 points[h * pointCountW + w + pointCountH * pointCountW] > hMax
                 )
            {
                points[h * pointCountW + w] = -1;
                points[h * pointCountW + w + pointCountH * pointCountW] = -1;
            }
        }

    // Interpolate pixels

    double param1, param2;
    int neigTLW, neigTLH;
    int newValR;
    int newValG;
    int newValB;
    int newCoordW, newCoordH;

    groupIdFgEnlarged = new int[wMax * hMax];

    enlargedForeground.fill(0);

    for ( int i = 0; i < wMax*hMax; i++)
        groupIdFgEnlarged[i] = 0;

    for (int  h = 0; h < pointCountH; h++ )
        for ( int w = 0; w < pointCountW; w++ )
        {
            if ( points[h * pointCountW + w] == -1 )
                continue;

            neigTLW = floor( points[h * pointCountW + w] );
            neigTLH = floor( points[h * pointCountW + w + pointCountH * pointCountW] );

            if ( neigTLW + 1 >= wMax || neigTLH +1 >= hMax )
                continue;

            param1 = points[h * pointCountW + w] - neigTLW;
            param2 = points[h * pointCountW + w + pointCountH * pointCountW] - neigTLH;


            if ( ( groupIdRight[ neigTLH * wMax + neigTLW ]  != 1 ) ||
                 ( groupIdRight[ (neigTLH+1) * wMax + neigTLW ] != 1 ) ||
                 ( groupIdRight[ neigTLH * wMax + (neigTLW+1) ]  != 1 ) ||
                 ( groupIdRight[ (neigTLH+1) * wMax + (neigTLW+1) ] != 1 ) )
                continue;

            newValR = qRed(onlyForeground.pixel(neigTLW, neigTLH)) * (1-param1) * (1-param2)
                    + qRed(onlyForeground.pixel(neigTLW + 1, neigTLH)) * (param1) * (1-param2)
                    + qRed(onlyForeground.pixel(neigTLW, neigTLH + 1)) * (1-param1) * (param2)
                    + qRed(onlyForeground.pixel(neigTLW + 1, neigTLH + 1)) * (param1) * (param2);

            newValG = qGreen(onlyForeground.pixel(neigTLW, neigTLH)) * (1-param1) * (1-param2)
                    + qGreen(onlyForeground.pixel(neigTLW + 1, neigTLH)) * (param1) * (1-param2)
                    + qGreen(onlyForeground.pixel(neigTLW, neigTLH + 1)) * (1-param1) * (param2)
                    + qGreen(onlyForeground.pixel(neigTLW + 1, neigTLH + 1)) * (param1) * (param2);

            newValB = qBlue(onlyForeground.pixel(neigTLW, neigTLH)) * (1-param1) * (1-param2)
                    + qBlue(onlyForeground.pixel(neigTLW + 1, neigTLH)) * (param1) * (1-param2)
                    + qBlue(onlyForeground.pixel(neigTLW, neigTLH + 1)) * (1-param1) * (param2)
                    + qBlue(onlyForeground.pixel(neigTLW + 1, neigTLH + 1)) * (param1) * (param2);

            newCoordW = centerOfMass.x() - (midPtW - w);
            newCoordH = centerOfMass.y() - (midPtH - h);

            if ( (newCoordW > 0) && (newCoordH > 0) && (newCoordW < wMax) && (newCoordH < hMax) )//&& (newValR > 0))
            {
                enlargedForeground.setPixel( newCoordW, newCoordH, qRgb(newValR, newValG, newValB) );
                groupIdFgEnlarged[newCoordH * wMax + newCoordW] = 1;
            }

        }

}

void ISegmentation::computeAlphaMatteMap()
{
    int hMax = depthMap.height();
    int wMax = depthMap.width();

    for ( int h = 0; h < hMax; h++)
        for ( int w = 0; w < wMax; w++)
            if ( groupIdRight[h*wMax + w] == 1 )
                alphaBlend.setPixel(w, h, qRgb(255, 255, 255));
            else
                alphaBlend.setPixel(w, h, 0);

    int matteRadius = (matteWidth - 1) / 2;

    int matteSize = matteWidth * matteWidth;

    // Now apply a low-pass filter, avarage in 3x3 neigborhood
    int tempSum;

    for ( int h = matteRadius; h < hMax-matteRadius; h++ )
        for ( int w = matteRadius; w < wMax-matteRadius; w++ )
        {
            tempSum = 0;

            for ( int i = -matteRadius; i <= matteRadius; i++ )
                for ( int j = -matteRadius; j <= matteRadius; j++ )
                    if ( groupIdRight[ (h+i) * wMax + w+j ] == 1 )
                        tempSum += 255;

            tempSum = tempSum / matteSize;

            alphaBlend.setPixel(w, h, qRgb(tempSum, tempSum, tempSum));

        }

}

void ISegmentation::changeDepthMode(int dMode)
{

    FGLocation = (double) dMode / 10;
    BGLocation = FGLocation;

    computeDepth();

}

void ISegmentation::changeAlphaMatte(int aMode)
{
    matteWidth = 2 * aMode + 5;

    computeDepth();
}

void ISegmentation::depthBasedRendering2()
{
    int wMax = depthMap.width();
    int hMax = depthMap.height();
    int wNew;
    int pixDepth;

    leftView.fill(0);
    rightView.fill(0);

    // Generate Left Image

    groupIdFgEnLeft = new int[wMax * hMax];

    for ( int i = 0; i < wMax*hMax; i++)
        groupIdFgEnLeft[i] = 0;


    for ( int w = 0; w < wMax; w++)
        for ( int h = 0; h < hMax; h++)
        {
            pixDepth = 1;
            wNew = w + FOC_LENGTH * HOR_TRANS / pixDepth - FGLocation * FOC_LENGTH * HOR_TRANS;
            if ( (wNew >= 0) && (wNew <= wMax) )
            {
                leftFg.setPixel(wNew, h, enlargedForeground.pixel(w, h));
                leftAlpha.setPixel(wNew, h, alphaBlend.pixel(w, h));
                if ( groupIdFgEnlarged[h * wMax + w] == 1)
                    groupIdFgEnLeft[ h * wMax + wNew ] = 1;
            }
        }

    for ( int w = 0; w < wMax; w++)
        for ( int h = 0; h < hMax; h++)
        {
            pixDepth = 255 * ( 1 - (double) h / (double) hMax ) + 1;
            wNew = w + FOC_LENGTH * HOR_TRANS / pixDepth - BGLocation * FOC_LENGTH * HOR_TRANS;
            if ( (wNew >= 0) && (wNew <= wMax) )
            {
                leftBg.setPixel(wNew, h, filledBackground.pixel(w, h));
            }
        }

    double alphaCoeff;
    int tempR, tempG, tempB;

    for ( int w = 0; w < wMax; w++)
        for ( int h = 0; h < hMax; h++)
        {
            if ( groupIdFgEnLeft[h * wMax + w] != 1 )
            {
                leftView.setPixel( w, h, leftBg.pixel(w,h));
                continue;
            }
            alphaCoeff = (double)qRed(leftAlpha.pixel(w,h)) / 255;
            tempR = alphaCoeff * (double)qRed(leftFg.pixel(w,h)) +
                    ( 1 - alphaCoeff ) * (double)qRed(leftBg.pixel(w,h));
            tempG = alphaCoeff * (double)qGreen(leftFg.pixel(w,h)) +
                    ( 1 - alphaCoeff ) * (double)qGreen(leftBg.pixel(w,h));
            tempB = alphaCoeff * (double)qBlue(leftFg.pixel(w,h)) +
                    ( 1 - alphaCoeff ) * (double)qBlue(leftBg.pixel(w,h));
            leftView.setPixel( w, h, qRgb(tempR, tempG, tempB));
        }

    // Generate Right Image

    for ( int w = 0; w < wMax; w++)
        for ( int h = 0; h < hMax; h++)
        {
            if ( groupIdFgEnlarged[h * wMax + w] != 1 )
            {
                rightView.setPixel( w, h, filledBackground.pixel(w,h));
                continue;
            }
            alphaCoeff = (double)qRed(alphaBlend.pixel(w,h)) / 255;
            tempR = alphaCoeff * (double)qRed(enlargedForeground.pixel(w,h)) +
                    ( 1 - alphaCoeff ) * (double)qRed(filledBackground.pixel(w,h));
            tempG = alphaCoeff * (double)qGreen(enlargedForeground.pixel(w,h)) +
                    ( 1 - alphaCoeff ) * (double)qGreen(filledBackground.pixel(w,h));
            tempB = alphaCoeff * (double)qBlue(enlargedForeground.pixel(w,h)) +
                    ( 1 - alphaCoeff ) * (double)qBlue(filledBackground.pixel(w,h));
            rightView.setPixel( w, h, qRgb(tempR, tempG, tempB));
        }


}

void ISegmentation::fillBackground2()
{
    int hMax = depthMap.height();
    int wMax = depthMap.width();

    int wRowMin, wRowMax;

    for ( int h = 0; h < hMax; h++ )
        for ( int w = 0; w < wMax; w++ )
            filledBackground.setPixel(w, h, onlyBackground.pixel(w, h));

    if ( groupIdRight[0] != 2 )
        filledBackground.setPixel(0, 0, qRgb(100,100,100));

    for ( int h = 1; h < hMax; h++ )
        if ( groupIdRight[h * wMax ] != 2 )
            filledBackground.setPixel(0, h, filledBackground.pixel(0, h-1));

    if ( groupIdRight[wMax-1] != 2 )
        filledBackground.setPixel(wMax-1, 0, qRgb(100,100,100));

    for ( int h = 1; h < hMax; h++ )
        if ( groupIdRight[h * wMax  + wMax - 1] != 2 )
            filledBackground.setPixel(wMax-1, h, filledBackground.pixel(wMax-1, h-1));

    for ( int h = 0; h < hMax; h++ )
    {
        wRowMin = 0;
        wRowMax = wMax;
        int w = 0;
        for ( ; ( groupIdRight[h * wMax + w ] == 2 ) && w < wMax-1; w++ );

        wRowMin = w;
        if ( wRowMin == 0 )
            wRowMin++;

        w = wMax-1;
        for ( ;  ( groupIdRight[h * wMax + w ] == 2 ) && w > 0; w-- );

        wRowMax = w;
        if ( wRowMax == wMax )
            wRowMax--;

        if ( wRowMin >= wRowMax )
            continue;

        w = wRowMin;
        for ( ; w < ( wRowMin + wRowMax ) / 2; w++)
            filledBackground.setPixel(w, h, filledBackground.pixel(wRowMin-1, h));

        w = wRowMax;
        for ( ; w >= ( wRowMin + wRowMax ) / 2; w--)
            filledBackground.setPixel(w, h, filledBackground.pixel(wRowMax+1, h));
    }

    for ( int h = 0; h < hMax; h++ )
        for ( int w = 0; w < wMax; w++ )
        {
            if ( groupIdRight[h * wMax + w] == 2)
                filledBackground.setPixel(w, h, onlyBackground.pixel(w, h));
        }

}

void ISegmentation::computeAlphaMatteMap2()
{
    int hMax = depthMap.height();
    int wMax = depthMap.width();


    for ( int h = 0; h < hMax; h++)
        for ( int w = 0; w < wMax; w++)
            if ( groupIdFgEnlarged[h*wMax + w] == 1 )
                alphaBlend.setPixel(w, h, qRgb(255, 255, 255));
            else
                alphaBlend.setPixel(w, h, 0);

    // enlarge matte radius with respect to FG enlargement scale

    int matteWidthTemp = ceil((double)matteWidth * DEFAULT_SCALE);

    if (!( matteWidthTemp % 2 ))
        matteWidthTemp++;

    int matteRadius = (matteWidthTemp - 1) / 2;

    int matteSize = matteWidthTemp * matteWidthTemp;

    // Now apply a low-pass filter
    int tempSum;

    for ( int h = matteRadius; h < hMax-matteRadius; h++ )
        for ( int w = matteRadius; w < wMax-matteRadius; w++ )
        {
            tempSum = 0;

            for ( int i = -matteRadius; i <= matteRadius; i++ )
                for ( int j = -matteRadius; j <= matteRadius; j++ )
                    if ( groupIdFgEnlarged[ (h+i) * wMax + w+j ] == 1 )
                        tempSum += 255;

            tempSum = tempSum / matteSize;

            alphaBlend.setPixel(w, h, qRgb(tempSum, tempSum, tempSum));

        }

}

void ISegmentation::changeFGEnlargementMode(int mode)
{
    FGEnlargementMode = mode;
    computeDepth();
}

void ISegmentation::changeBlurMode(int mode)
{
    blurMode = mode;
    computeDepth();
}

void ISegmentation::blurBG()
{
    int wMax = depthMap.width();
    int hMax = depthMap.height();

    QImage imBuffer = filledBackground;

    int blurWidth = 7;

    int blurRadius = (blurWidth - 1) / 2;

    int blurSize = blurWidth * blurWidth;

    int tempSumR, tempSumG, tempSumB;

    for ( int h = blurRadius; h < hMax-blurRadius; h++ )
        for ( int w = blurRadius; w < wMax-blurRadius; w++ )
        {
            tempSumR = 0;
            tempSumG = 0;
            tempSumB = 0;

            for ( int i = -blurRadius; i <= blurRadius; i++ )
                for ( int j = -blurRadius; j <= blurRadius; j++ )
                {
                    tempSumR += qRed  ( imBuffer.pixel(w+j, h+i) );
                    tempSumG += qGreen( imBuffer.pixel(w+j, h+i) );
                    tempSumB += qBlue ( imBuffer.pixel(w+j, h+i) );
                }


            tempSumR = tempSumR / blurSize;
            tempSumG = tempSumG / blurSize;
            tempSumB = tempSumB / blurSize;

            filledBackground.setPixel(w, h, qRgb(tempSumR, tempSumG, tempSumB));

        }

}

void ISegmentation::blurFG()
{

    int wMax = depthMap.width();
    int hMax = depthMap.height();

    int FGNeigh = (11 - 1) / 2;

    int blurWidth = 5;
    int blurRadius = (blurWidth - 1) / 2;
    int blurSize = blurWidth * blurWidth;
    int tempSumR, tempSumG, tempSumB;

    // Now apply a low-pass filter
    int tempSum;

    for ( int h = FGNeigh; h < hMax-FGNeigh; h++ )
        for ( int w = FGNeigh; w < wMax-FGNeigh; w++ )
        {
            tempSum = 0;

            if ( groupIdRight[h * wMax + w] == 2 )
                continue;

            for ( int i = -FGNeigh; i <= FGNeigh; i++ )
                for ( int j = -FGNeigh; j <= FGNeigh; j++ )
                    if ( groupIdRight[(h+i) * wMax + w+j] == 1 )
                        tempSum ++;


            // Blur if distance to BG is less than 5
            if ( tempSum < 25 )
            {
                tempSumR = 0;
                tempSumG = 0;
                tempSumB = 0;

                for ( int i = -blurRadius; i <= blurRadius; i++ )
                    for ( int j = -blurRadius; j <= blurRadius; j++ )
                    {
                        tempSumR += qRed  ( inputImage.pixel(w+j, h+i) );
                        tempSumG += qGreen( inputImage.pixel(w+j, h+i) );
                        tempSumB += qBlue ( inputImage.pixel(w+j, h+i) );
                    }


                tempSumR = tempSumR / blurSize;
                tempSumG = tempSumG / blurSize;
                tempSumB = tempSumB / blurSize;

                onlyForeground.setPixel(w, h, qRgb(tempSumR, tempSumG, tempSumB));

            }

        }
}

void ISegmentation::modifyFGMap()
{
    int wMax = depthMap.width();
    int hMax = depthMap.height();

    // 1: FG, 2: BG

    for ( int h = 0; h < hMax; h++)
        for ( int w = 2; w < wMax; w++ )
        {
            if ( (groupIdRight[ h * wMax + w-1 ] == 2)
                 &&
                 (groupIdRight[ h * wMax + w ] == 1) )
            {
                groupIdRight[ h * wMax + w-1 ] = 1;
                groupIdRight[ h * wMax + w-2 ] = 1;
            }
        }

    for ( int h = 0; h < hMax; h++)
        for ( int w = wMax-2; w > 0; w-- )
        {
            if ( (groupIdRight[ h * wMax + w+1 ] == 2)
                 &&
                 (groupIdRight[ h * wMax + w ] == 1) )
            {
                groupIdRight[ h * wMax + w+1 ] = 1;
                groupIdRight[ h * wMax + w+2 ] = 1;
            }
        }

}
