#ifndef INTERACTIVESEGMENTATION_H
#define INTERACTIVESEGMENTATION_H

#include "StructDefs.h"
#include "grabcut.h"
#include "SegmentationThread.h"
#include <QImage>
#include <vector>
class ISegmentation
{
public:
    typedef enum _METHOD{
        BATCH,
        COLORING,
        NOGCUT
    }METHOD;

private:
    GrabCut gCut;
    bool* brushMap;

    bool isFirstImage;
    QImage inputImage;

    void createGraphStructure();
    void initOSImage();
    void initOverSegment();
public:
    METHOD mSegmentationMethod;
    RegionNode* regionNode;
    SegmentationThread* sT;
    int *SegmentID; //Segment ID of each pixel, two dimensional arra cat to 1 dimension

    QImage depthMap;
    QImage rightView;
    QImage leftView;
    QImage anaglyphImage;
    QImage onlyForeground;
    QImage onlyBackground;
    QImage enlargedForeground;
    QImage filledBackground;
    QImage alphaBlend;
    QImage leftBg;
    QImage leftFg;
    QImage leftAlpha;

    int *groupIdLeft;
    int *groupIdRight;
    int *groupIdFgEnlarged;
    int *groupIdFgEnLeft;

    double FGLocation;
    double BGLocation;

    int matteWidth;
    int FGEnlargementMode;
    int blurMode;
    bool isMerge;

    std::vector<int> queueBG;
    std::vector<int> queueFG;

    Params param;

    ISegmentation();
    void setSegmentationMethod(METHOD type);
    void reset();
    void insertPoint(int x,int y,bool isFG);
    void setImage(QImage* im);
    void overSegment();
    void drawOSBoundaryOnImage(QImage* im);
    void segmentBatch();


    void computeDepth();
    void changeView(QImage* im,int which);
    void depthBasedRendering();
    void depthBasedRendering2();
    void fillBackground();
    void fillBackground2();
    void createAnaglyphImage();
    void enlargeForeground(double scale);
    void linearAlphaBlender();
    void computeAlphaMatteMap();
    void computeAlphaMatteMap2();
    void changeDepthMode(int dMode);
    void changeAlphaMatte(int aMode);
    void changeFGEnlargementMode(int mode);
    void blurBG();
    void blurFG();
    void modifyFGMap();
    void changeBlurMode(int mode);
    void mergeSP(int id);
    int getMinDistSP(int id);
};

#endif // INTERACTIVESEGMENTATION_H
