#ifndef STRUCTDEFS_H
#define STRUCTDEFS_H

#include <QWaitCondition>
typedef struct _params
{
        int     width;
        int     height;
        int     blockSize;
        int     segmentNumber;

} Params;


typedef struct _region_Node
{
    int     neighborID[10];// max 10 neighbours allowed.
    int     neighborNum;
    int     groupID;               // 0 Not decided, 1 Foreground, 2 Background
    int     segmentSize;

    int     minX;
    int     minY;
    int     maxX;
    int     maxY;

    float   meanR;
    float   meanG;
    float   meanB;

    float   neighborDistance[10];
    int     boundSize[10];
}RegionNode;

#endif // STRUCTDEFS_H
