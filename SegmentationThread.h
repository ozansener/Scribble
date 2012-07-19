#ifndef SEGMENTATIONTHREAD_H
#define SEGMENTATIONTHREAD_H
#include "StructDefs.h"
#include "grabcut.h"
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <vector>
class ScribbleArea;
class SegmentationThread:public QThread{
    Q_OBJECT
public:
    ScribbleArea* wid;
    bool threadStop;
    SegmentationThread(QObject* parent=0);
    ~SegmentationThread();
    void ResetVec();
    void reset();
    void setWidget(ScribbleArea* w);
    void setQueues(std::vector<int>* qB,std::vector<int>* qF);
    void setGCUT(GrabCut* grCut);
public slots:
//    void add2FgGMM(int ID);
//    void add2BgGMM(int ID);
//    void runGrabCut();
protected:
    void run();
private:
    int posBG;
    int posFG;

    GrabCut* gCut;
    QMutex mutexMain;
    QMutex mutexImage;
    QMutex mutexRN;

    std::vector<int>* queueBG;
    std::vector<int>* queueFG;
};
#endif // SEGMENTATIONTHREAD_H
