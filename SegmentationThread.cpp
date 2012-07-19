#include"SegmentationThread.h"
#include"scribblearea.h"
#include <iostream>

SegmentationThread::SegmentationThread(QObject *parent){
    posBG = 0;
    posFG = 0;
    mutexMain.lock();
    mutexMain.unlock();
}

void SegmentationThread::ResetVec(){
    mutexMain.lock();
    posBG = 0;
    posFG = 0;
    gCut->reset();
    queueBG->clear();
    queueFG->clear();
    mutexMain.unlock();
}

SegmentationThread::~SegmentationThread(){
  //  wait();
}



void SegmentationThread::run(){

    static int cnt = 0;
    int nFGSize;
    int nBGSize;
    int size;
    std::vector<int> FG;
    std::vector<int> BG;

    while(1){
        size = queueBG->size()+queueFG->size();
        if(size!=(posBG+posFG))
        {
            if(queueFG->size()!=posFG){
                nFGSize = queueFG->size();
                FG.resize(nFGSize - posFG);
                for(int i=posFG;i<nFGSize;i++)
                    FG[i-posFG]=(*queueFG)[i];

                posFG = nFGSize;
            }
            if(queueBG->size()!=posBG){
                nBGSize = queueBG->size();
                BG.resize(nBGSize - posBG);
                for(int i=posBG;i<nBGSize;i++)
                    BG[i-posBG]=(*queueBG)[i];
                posBG = nBGSize;
            }

            for(int i=0;i<FG.size();i++)
                gCut->add2FG(FG[i]);
            for(int i=0;i<BG.size();i++)
                gCut->add2BG(BG[i]);

//             if((cnt%2)==1)
            cnt++;
            gCut->runGrabCut();
            //mutexImage.lock();
            //mutexImage.unlock();
            FG.clear();
            BG.clear();
            wid->drawResult();
            msleep(20);


        }
        msleep(40);

    }

}



void SegmentationThread::reset(){
    mutexMain.lock();
    gCut->reset();
    queueBG->clear();
    posBG=0;
    posFG=0;
    queueFG->clear();
    mutexMain.unlock();
}

void SegmentationThread::setWidget(ScribbleArea* w){
    wid = w;
}


void SegmentationThread::setQueues(std::vector<int>* qB,std::vector<int>* qF){
    queueBG = qB;
    queueFG = qF;
}

void SegmentationThread::setGCUT(GrabCut *grCut){
    gCut=grCut;
}
