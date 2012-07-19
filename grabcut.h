#ifndef GRABCUT_H
#define GRABCUT_H
#include <vector>
#include "graph.h"
#include "GMM.h"
#include "StructDefs.h"
#include <QImage>
typedef Graph<float,float,float> FloatGraph;

typedef struct _EM{
    float RGBMeanFG[3];
    float RGBMeanBG[3];

    float RGBMoment2FG[3];
    float RGBMoment2BG[3];

    float RGBVarFG[3];
    float RGBVarBG[3];

    float* EnergyBG;
    float* EnergyFG;
    int* RegionNumPcsdPixel;

}EM;

class GrabCut
{
private:
    bool firstRun;
    bool isROC;
    float betaMean;
    std::vector<int> foregroundGraph;
    std::vector<int> backgroundGraph;
    FloatGraph* gr;
    int segmentNumber;
    RegionNode **regionNode;
    int *segmentIDs;
    int *oldIDs;
    QImage* image;
    EM gMM;
    EM* curGMM;
    EM* oldGMM;
    GMM fgGMM;
    GMM bgGMM;
    float *bgVec;
    int bBoxXmin;
    int bBoxXmax;
    int bBoxYmin;
    int bBoxYmax;
    float e1const;

public:
    int width;
    int height;
    GrabCut();
    void setSegmentID(int *s);
    void setImage(QImage* im);
    void add2FG(int toAdd);
    void add2BG(int toAdd);
    void setSegmentNumber(int number);
    void setRegionNode(RegionNode** rN);
    int getResult(int segmentID);
    void runGrabCut();
    void runGrabCutNoBG();
    void runResidualGrabCutNoBG();
    void reset();
    void runResidualGrabCutFirstRun();

};

#endif // GRABCUT_H
