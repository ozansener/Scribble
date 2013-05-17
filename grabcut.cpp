#include "grabcut.h"
#include <QColor>
#include <math.h>
#include <fstream>
#include <iostream>
#include <algorithm>

#define LAMBDA 1
#define E1CONST 1
#define E2CONST 0.5

using namespace std;
GrabCut::GrabCut()
{
    fgGMM.initializeAndClear(3,5);
    bgGMM.initializeAndClear(3,5);
    foregroundGraph.clear();
    backgroundGraph.clear();
    firstRun = true;
    isROC = true;
}

void GrabCut::add2FG(int toAdd){

    if(isROC)
    {
        bBoxXmin = width;
        bBoxXmax = 0;
        bBoxYmin = height;
        bBoxYmax = 0;
        isROC = false;
    }

    if((*regionNode)[toAdd].minX<bBoxXmin)
        bBoxXmin=(*regionNode)[toAdd].minX;
    if((*regionNode)[toAdd].maxX>bBoxXmax)
        bBoxXmax=(*regionNode)[toAdd].maxX;
    if((*regionNode)[toAdd].minY<bBoxYmin)
        bBoxYmin=(*regionNode)[toAdd].minY;
    if((*regionNode)[toAdd].maxY>bBoxYmax)
        bBoxYmax=(*regionNode)[toAdd].maxY;


    if(toAdd>segmentNumber)
        return;
    for(int i=0;i<((int)foregroundGraph.size());i++)
        if(foregroundGraph[i]==toAdd)
            return;
    foregroundGraph.push_back(toAdd);

    int size= (*regionNode)[toAdd].segmentSize;
    int cnt=0;
    float* inVec = new float[size*3];
    QColor c;
    for(int w=(*regionNode)[toAdd].minX;w<(*regionNode)[toAdd].maxX;w=w+4)
       for(int h=(*regionNode)[toAdd].minY;h<(*regionNode)[toAdd].maxY;h=h+4)
       {
            if(segmentIDs[h*width +w]==toAdd)
            {
                c = image->pixel(w,h);
                inVec[cnt*3+0]=c.red();
                inVec[cnt*3+1]=c.green();
                inVec[cnt*3+2]=c.blue();
                cnt++;
            }
       }
    fgGMM.insertData(inVec,cnt);
    fgGMM.iterateGMM(3);
    delete [] inVec;
}
void GrabCut::add2BG(int toAdd){
    backgroundGraph.push_back(toAdd);

    for(int i=0;i<((int)backgroundGraph.size())-1;i++)
        if(backgroundGraph[i]==toAdd)
            return;

    int size=(*regionNode)[toAdd].segmentSize;
    int cnt=0;
    float* inVec = new float[size*3];
    QColor c;
    for(int w=(*regionNode)[toAdd].minX;w<(*regionNode)[toAdd].maxX;w=w+4)
       for(int h=(*regionNode)[toAdd].minY;h<(*regionNode)[toAdd].maxY;h=h+4)
       {
            if(segmentIDs[h*width+w]==toAdd)
            {
                c = image->pixel(w,h);
                inVec[cnt*3+0]=c.red();
                inVec[cnt*3+1]=c.green();
                inVec[cnt*3+2]=c.blue();
                cnt++;
            }
       }
    bgGMM.insertData(inVec,cnt);
    bgGMM.iterateGMM(3);
    delete [] inVec;
}
void GrabCut::setSegmentNumber(int number){
    segmentNumber = number;
}
void GrabCut::setRegionNode(RegionNode** rN){
    regionNode = rN;
}
int GrabCut::getResult(int segmentID){
    if((*regionNode)[segmentID].groupID==1)
        return 0;
    else
        return 1;
}
void GrabCut::runGrabCut(){


    if(backgroundGraph.size()==0)
    {
        runResidualGrabCutNoBG();
   //     runGrabCutNoBG();
        return;
    }
    /*
    QColor c;

    gr = new FloatGraph(segmentNumber,segmentNumber*10);


    for(int i=0;i<segmentNumber;i++)
        (*regionNode)[i].groupID = 0;


    for(int i=0;i<foregroundGraph.size();i++)
        (*regionNode)[foregroundGraph[i]].groupID = 1;

    for(int j=0;j<backgroundGraph.size();j++)
        (*regionNode)[backgroundGraph[j]].groupID = 2;

    for(int i=0;i<segmentNumber;i++)
        gr->add_node();


    gMM.EnergyBG = new float[segmentNumber];
    gMM.EnergyFG = new float[segmentNumber];
    gMM.RegionNumPcsdPixel = new int[segmentNumber];
    for(int i=0;i<segmentNumber;i++)
     {
         gMM.RegionNumPcsdPixel[i]=0;
         gMM.EnergyBG[i]=0;
         gMM.EnergyFG[i]=0;
     }
    int rId;
    float inve[3];
    for(int w=0;w<width;w=w+2)
    {
        for(int h=0;h<height;h=h+2){
            c = image->pixel(w,h);
            inve[0]=c.red();
            inve[1]=c.green();
            inve[2]=c.blue();
            rId = segmentIDs[w+h*width];
            gMM.EnergyBG[rId]+=bgGMM.getLikelihood(inve);
            gMM.EnergyFG[rId]+=fgGMM.getLikelihood(inve);
            gMM.RegionNumPcsdPixel[rId]++;
        }
    }
    for(int i=0;i<segmentNumber;i++)
     {
         if(gMM.RegionNumPcsdPixel[i]==0)
         {
            gMM.EnergyBG[i]=0;
            gMM.EnergyFG[i]=0;
         }else
         {
             gMM.EnergyBG[i]=gMM.EnergyBG[i]/((float)gMM.RegionNumPcsdPixel[i]);
             gMM.EnergyFG[i]=gMM.EnergyFG[i]/((float)gMM.RegionNumPcsdPixel[i]);
         }
     }

    float betaMean = 0;
    int betaNum=0;

    float meane1=0;
    float incR;
    for(int i=0;i<segmentNumber;i++)
    {
        meane1+=gMM.EnergyBG[i];
        meane1+=gMM.EnergyFG[i];

        for(int j = 0;j<(*regionNode)[i].neighborNum;j++)
        {

            incR = ((*regionNode)[i].meanR - (*regionNode)[(*regionNode)[i].neighborID[j]].meanR)*((*regionNode)[i].meanR - (*regionNode)[(*regionNode)[i].neighborID[j]].meanR);
            incR += ((*regionNode)[i].meanG - (*regionNode)[(*regionNode)[i].neighborID[j]].meanG)*((*regionNode)[i].meanG - (*regionNode)[(*regionNode)[i].neighborID[j]].meanG);
            incR += ((*regionNode)[i].meanB - (*regionNode)[(*regionNode)[i].neighborID[j]].meanB)*((*regionNode)[i].meanB - (*regionNode)[(*regionNode)[i].neighborID[j]].meanB);

            //incR = (*regionNode)[i].neighborDistance[j];
            betaMean += incR;
            betaNum++;
        }
    }


    meane1=meane1/(2*segmentNumber);
    betaMean = betaMean/betaNum;
    //Parameters !!!
    float lambda = 1;//If too low let changes on t-weights

    float e1const = 70*(1/meane1);
    float e2const = 1; //Less than 2

    float beta = 1/betaMean;
    float capp;

    for(int i=0;i<segmentNumber;i++)
    {
       //   if((*regionNode)[i].groupID==1)
       //      gr->add_tweights(i,0,20);
       //  else if((*regionNode)[i].groupID==2)
       //      gr->add_tweights(i,20,0);
       //   else
              gr->add_tweights(i,e1const*gMM.EnergyBG[i],e1const*gMM.EnergyFG[i]);


        for(int j=0;j<(*regionNode)[i].neighborNum;j++)
        {

            capp = ((*regionNode)[i].meanR - (*regionNode)[(*regionNode)[i].neighborID[j]].meanR)*((*regionNode)[i].meanR - (*regionNode)[(*regionNode)[i].neighborID[j]].meanR);
            capp += ((*regionNode)[i].meanG - (*regionNode)[(*regionNode)[i].neighborID[j]].meanG)*((*regionNode)[i].meanG - (*regionNode)[(*regionNode)[i].neighborID[j]].meanG);
            capp += ((*regionNode)[i].meanB - (*regionNode)[(*regionNode)[i].neighborID[j]].meanB)*((*regionNode)[i].meanB - (*regionNode)[(*regionNode)[i].neighborID[j]].meanB);

            capp = exp((-1)*capp*beta*lambda);
            gr->add_edge(i,(*regionNode)[i].neighborID[j],e2const*capp,e2const*capp);
        //    of<<i<<" "<<(*regionNode)[i].neighborID[j]<<" "<<e2const*capp<<" "<<e2const*capp<<endl;
        }


    }
    ofstream of1;
    of1.open("GraphDumpBefore.txt");
    for(int y=0;y<height;y++)
    {
        for(int x=0;x<width;x++)
            of1<<gr->get_trcap(segmentIDs[y*width+x])<<" ";
        of1<<endl;
    }
    of1.close();

    gr->maxflow();



    for(int i=0;i<segmentNumber;i++)
        if(gr->what_segment(i)==FloatGraph::SOURCE)
            (*regionNode)[i].groupID = 2;
        else
            (*regionNode)[i].groupID = 1;


    //    gr->reset();
    delete gr;
    delete [] gMM.EnergyFG;
    delete [] gMM.EnergyBG;
    delete [] gMM.RegionNumPcsdPixel;
*/
}


void GrabCut::runGrabCutNoBG(){

    static int intId = 0;
    /*
    for(int i=0;i<segmentNumber;i++)
        if(((*regionNode)[i].maxX<bBoxXmax)&&((*regionNode)[i].minX>bBoxXmin)&&((*regionNode)[i].maxY<bBoxYmax)&&((*regionNode)[i].minY>bBoxYmin))
            (*regionNode)[i].groupID = 1;

    return;
    */
    if(firstRun)
    {
        bgVec = new float[3*width*height];

    }
    QColor c;

    gr = new FloatGraph(segmentNumber,segmentNumber*10);

    for(int i=0;i<segmentNumber;i++)
        (*regionNode)[i].groupID = 3;

    for(int x=max(0,bBoxXmin-20);x<min(width,bBoxXmax+20);x++)
        for(int y=max(0,bBoxYmin-20);y<min(height,bBoxYmax+20);y++)
            (*regionNode)[segmentIDs[y*width+x]].groupID=2;

    int nID;
    for(int i=0;i<foregroundGraph.size();i++)
    {
        (*regionNode)[foregroundGraph[i]].groupID = 1;
        for(int j=0;j<(*regionNode)[foregroundGraph[i]].neighborNum;j++)
        {
            nID = (*regionNode)[foregroundGraph[i]].neighborID[j] ;
            (*regionNode)[nID].groupID=0;
          //  for(int k=0;k<(*regionNode)[nID].neighborNum;k++)
          //      (*regionNode)[(*regionNode)[nID].neighborID[k]].groupID=0;
        }
    }


 //   return;
    for(int i=0;i<foregroundGraph.size();i++)
        (*regionNode)[foregroundGraph[i]].groupID = 1;

    for(int i=0;i<segmentNumber;i++)
        gr->add_node();


    gMM.EnergyBG = new float[segmentNumber];
    gMM.EnergyFG = new float[segmentNumber];
    gMM.RegionNumPcsdPixel = new int[segmentNumber];
    for(int i=0;i<segmentNumber;i++)
     {
         gMM.RegionNumPcsdPixel[i]=0;
         gMM.EnergyBG[i]=0;
         gMM.EnergyFG[i]=0;
     }
    int rId;
    float inve[3];
    for(int w=0;w<width;w=w+6)
    {
        for(int h=0;h<height;h=h+6)
        {
            c = image->pixel(w,h);
            inve[0]=c.red();
            inve[1]=c.green();
            inve[2]=c.blue();
            rId = segmentIDs[w+h*width];
            gMM.EnergyFG[rId]+=fgGMM.getLikelihood(inve);
            gMM.RegionNumPcsdPixel[rId]++;
        }
    }


  //  float* gmm2Sort=new float[segmentNumber];
    for(int i=0;i<segmentNumber;i++)
     {
         if(gMM.RegionNumPcsdPixel[i]==0)
         {
             gMM.EnergyFG[i]=-1;
             //gmm2Sort[i]=1e6;
         }
         else
         {
             gMM.EnergyFG[i]=gMM.EnergyFG[i]/((float)gMM.RegionNumPcsdPixel[i]);
             //gmm2Sort[i]=gMM.EnergyFG[i];
         }
     }

//    sort(gmm2Sort,gmm2Sort+segmentNumber);
//    float thresh = gmm2Sort[(int)(segmentNumber/2)];//(mult*sqrt(probSumVar));
//    delete [] gmm2Sort;

/*


    for(int j=0;j<segmentNumber;j++)
    {
        if((*regionNode)[j].groupID!=1)
            if((gMM.EnergyFG[j])<thresh)
            {
                if(gMM.EnergyFG[j]!=-1)
                {
                    (*regionNode)[j].groupID = 2;
                    bgS+=(*regionNode)[j].segmentSize;
                }
            }
    }
    */


    int bCount =0;

    for(int w=0;w<width;w=w+16)
        for(int h=0;h<height;h=h+16){
            if((*regionNode)[ segmentIDs[h*width + w]].groupID==3){
                c = image->pixel(w,h);
                bgVec[bCount*3+0]= c.red();
                bgVec[bCount*3+1]= c.green();
                bgVec[bCount*3+2]= c.blue();
                bCount++;
            }
        }

    for(int w=0;w<width;w=w+8)
        for(int h=0;h<height;h=h+8){
            if((*regionNode)[ segmentIDs[h*width + w]].groupID==2){
                c = image->pixel(w,h);
                bgVec[bCount*3+0]= c.red();
                bgVec[bCount*3+1]= c.green();
                bgVec[bCount*3+2]= c.blue();
                bCount++;
            }
        }
    bgGMM.clear();
    bgGMM.insertData(bgVec,bCount);
    if(firstRun)
    {
        bgGMM.iterateGMM(15);
        firstRun = false;
    }
    else
    {
        bgGMM.initLastMean();
        bgGMM.iterateGMM(3);
    }
    for(int i=0;i<segmentNumber;i++)
     {
         gMM.RegionNumPcsdPixel[i]=0;
         gMM.EnergyBG[i]=0;
     }
    for(int w=0;w<width;w=w+6)
        for(int h=0;h<height;h=h+6){
            c = image->pixel(w,h);
            inve[0]=c.red();
            inve[1]=c.green();
            inve[2]=c.blue();
            rId = segmentIDs[w+h*width];
            gMM.EnergyBG[rId]+=bgGMM.getLikelihood(inve);
            gMM.RegionNumPcsdPixel[rId]++;
        }

    for(int i=0;i<segmentNumber;i++)
     {
         if(gMM.RegionNumPcsdPixel[i]==0)
             gMM.EnergyBG[i]=0;
         else
             gMM.EnergyBG[i]=gMM.EnergyBG[i]/((float)gMM.RegionNumPcsdPixel[i]);
     }

    for(int i=0;i<segmentNumber;i++)
        if(gMM.EnergyFG[i]==-1)
            gMM.EnergyFG[i]=0;

    float betaMean = 0;
    int betaNum=0;

    float meane1=0;
    float incR;
    for(int i=0;i<segmentNumber;i++)
    {
        meane1+=gMM.EnergyBG[i];
        meane1+=gMM.EnergyFG[i];

        for(int j = 0;j<(*regionNode)[i].neighborNum;j++)
        {

            incR = ((*regionNode)[i].meanR - (*regionNode)[(*regionNode)[i].neighborID[j]].meanR)*((*regionNode)[i].meanR - (*regionNode)[(*regionNode)[i].neighborID[j]].meanR);
            incR += ((*regionNode)[i].meanG - (*regionNode)[(*regionNode)[i].neighborID[j]].meanG)*((*regionNode)[i].meanG - (*regionNode)[(*regionNode)[i].neighborID[j]].meanG);
            incR += ((*regionNode)[i].meanB - (*regionNode)[(*regionNode)[i].neighborID[j]].meanB)*((*regionNode)[i].meanB - (*regionNode)[(*regionNode)[i].neighborID[j]].meanB);

           // incR = (*regionNode)[i].neighborDistance[j];
            betaMean += incR;
            betaNum++;
        }
    }



    meane1=meane1/(2*segmentNumber);
    betaMean = betaMean/betaNum;
    //Parameters !!!
/*
    float lambda = 1.5;

    float e1const = 1*(1/meane1);
    float e2const = 1.6;
*/
    //Parameters !!!
    float lambda = LAMBDA;//If too low let changes on t-weights

    float e1const = E1CONST *(1/meane1);
    float e2const = E2CONST; //Less than 2

    /*
    float lambda = 1.5;//If too low let changes on t-weights, consider to change this sentence :)

    float e1const = 2*(1/meane1);
    float e2const = 1; //Less than 2

    */
    float beta = 1/betaMean;
    float capp;
    ofstream off;
    char fileName[512];
    sprintf(fileName,"interaction%d.txt",intId++);
    off.open(fileName);
    float capp2;
    off<<segmentNumber<<endl;
    for(int i=0;i<segmentNumber;i++)
    {
         if((*regionNode)[i].groupID==1)
          {
              off<<0<<" "<<100<<endl;
             gr->add_tweights(i,0,100);//0.5 * 10 + 1
          }
          else if((*regionNode)[i].groupID==3)
            {
              off<<100<<" "<<0<<endl;
              gr->add_tweights(i,100,0);
          }
          else
          {
              gr->add_tweights(i,e1const*gMM.EnergyBG[i],e1const*gMM.EnergyFG[i]);
             // off<<e1const*gMM.EnergyBG[i]<<" "<<e1const*gMM.EnergyFG[i]<<endl;

          }
          //off<<(*regionNode)[i].neighborNum<<endl;
          for(int j=0;j<(*regionNode)[i].neighborNum;j++)
          {
              capp = ((*regionNode)[i].meanR - (*regionNode)[(*regionNode)[i].neighborID[j]].meanR)*((*regionNode)[i].meanR - (*regionNode)[(*regionNode)[i].neighborID[j]].meanR);
              capp += ((*regionNode)[i].meanG - (*regionNode)[(*regionNode)[i].neighborID[j]].meanG)*((*regionNode)[i].meanG - (*regionNode)[(*regionNode)[i].neighborID[j]].meanG);
              capp += ((*regionNode)[i].meanB - (*regionNode)[(*regionNode)[i].neighborID[j]].meanB)*((*regionNode)[i].meanB - (*regionNode)[(*regionNode)[i].neighborID[j]].meanB);

              capp2 = capp;
              capp = exp((-1)*capp*beta*lambda);
              gr->add_edge(i,(*regionNode)[i].neighborID[j],e2const*capp,e2const*capp);
              off<<(*regionNode)[i].neighborID[j]<<" "<<e2const*capp<<endl;
          }

    }
    off.close();
    gr->maxflow();
/*
    for(int i=0;i<segmentNumber;i++)
        if(gMM.EnergyBG[i]>=gMM.EnergyFG[i])
            (*regionNode)[i].groupID = 2;
        else
            (*regionNode)[i].groupID = 1;
*/


    for(int i=0;i<segmentNumber;i++)
        if(gr->what_segment(i)==FloatGraph::SOURCE)
            (*regionNode)[i].groupID = 2;
        else
            (*regionNode)[i].groupID = 1;  firstRun = false;

    delete gr;
    delete [] gMM.EnergyFG;
    delete [] gMM.EnergyBG;
    delete [] gMM.RegionNumPcsdPixel;
}





void GrabCut::setImage(QImage* im){
    image = im;
}

void GrabCut::reset(){
    bgGMM.clear();
    fgGMM.clear();
    foregroundGraph.clear();
    backgroundGraph.clear();
}

void GrabCut::setSegmentID(int *s){
    segmentIDs = s;
}


void GrabCut::runResidualGrabCutFirstRun(){
    bgVec = new float[3*width*height];
    gr = new FloatGraph(segmentNumber,segmentNumber*10);
    for(int i=0;i<segmentNumber;i++)
        gr->add_node();
    curGMM = new EM;
    oldGMM = new EM;

    curGMM->EnergyBG = new float[segmentNumber];
    curGMM->EnergyFG = new float[segmentNumber];

    oldGMM->EnergyFG = new float[segmentNumber];
    oldGMM->EnergyBG = new float[segmentNumber];

    oldGMM->RegionNumPcsdPixel = new int[segmentNumber];
    curGMM->RegionNumPcsdPixel = new int[segmentNumber];

    oldIDs = new int[segmentNumber];

    for(int i=0;i<segmentNumber;i++)
     {
        curGMM->EnergyBG[i]=0;
        curGMM->EnergyFG[i]=0;
        curGMM->RegionNumPcsdPixel[i]=0;

        oldGMM->EnergyBG[i]=0;
        oldGMM->EnergyFG[i]=0;
        oldGMM->RegionNumPcsdPixel[i]=0;

        oldIDs[i]=0;
    }

    QColor c;

    for(int i=0;i<segmentNumber;i++)
        (*regionNode)[i].groupID = 3;

    for(int x=max(0,bBoxXmin-20);x<min(width,bBoxXmax+20);x++)
        for(int y=max(0,bBoxYmin-20);y<min(height,bBoxYmax+20);y++)
            (*regionNode)[segmentIDs[y*width+x]].groupID=2;

    int nID;
    for(int i=0;i<foregroundGraph.size();i++)
    {
        (*regionNode)[foregroundGraph[i]].groupID = 1;
        for(int j=0;j<(*regionNode)[foregroundGraph[i]].neighborNum;j++)
        {
            nID = (*regionNode)[foregroundGraph[i]].neighborID[j] ;
            (*regionNode)[nID].groupID=0;
        }
    }

    for(int i=0;i<foregroundGraph.size();i++)
        (*regionNode)[foregroundGraph[i]].groupID = 1;


    int rId;
    float inve[3];
    for(int w=0;w<width;w=w+6)
    {
        for(int h=0;h<height;h=h+6)
        {
            c = image->pixel(w,h);
            inve[0]=c.red();
            inve[1]=c.green();
            inve[2]=c.blue();
            rId = segmentIDs[w+h*width];
            curGMM->EnergyFG[rId]+=fgGMM.getLikelihood(inve);
            curGMM-> RegionNumPcsdPixel[rId]++;
        }
    }

    for(int i=0;i<segmentNumber;i++)
     {
         if(curGMM->RegionNumPcsdPixel[i]==0)
         {
             curGMM->EnergyFG[i]=-1;
         }
         else
         {
             curGMM->EnergyFG[i]=curGMM->EnergyFG[i]/((float)curGMM->RegionNumPcsdPixel[i]);
         }
     }

    int bCount =0;
    for(int w=0;w<width;w=w+16)
        for(int h=0;h<height;h=h+16){
            if((*regionNode)[ segmentIDs[h*width + w]].groupID==3){
                c = image->pixel(w,h);
                bgVec[bCount*3+0]= c.red();
                bgVec[bCount*3+1]= c.green();
                bgVec[bCount*3+2]= c.blue();
                bCount++;
            }
        }

    for(int w=0;w<width;w=w+8)
        for(int h=0;h<height;h=h+8){
            if((*regionNode)[ segmentIDs[h*width + w]].groupID==2){
                c = image->pixel(w,h);
                bgVec[bCount*3+0]= c.red();
                bgVec[bCount*3+1]= c.green();
                bgVec[bCount*3+2]= c.blue();
                bCount++;
            }
        }
    bgGMM.clear();
    bgGMM.insertData(bgVec,bCount);
    bgGMM.iterateGMM(15);
    for(int i=0;i<segmentNumber;i++)
     {
         curGMM->RegionNumPcsdPixel[i]=0;
     }
    for(int w=0;w<width;w=w+6)
        for(int h=0;h<height;h=h+6){
            c = image->pixel(w,h);
            inve[0]=c.red();
            inve[1]=c.green();
            inve[2]=c.blue();
            rId = segmentIDs[w+h*width];
            curGMM->EnergyBG[rId]+=bgGMM.getLikelihood(inve);
            curGMM->RegionNumPcsdPixel[rId]++;
        }

    for(int i=0;i<segmentNumber;i++)
     {
         if(curGMM->RegionNumPcsdPixel[i]==0)
             curGMM->EnergyBG[i]=0;
         else
             curGMM->EnergyBG[i]=curGMM->EnergyBG[i]/((float)curGMM->RegionNumPcsdPixel[i]);
     }
    for(int i=0;i<segmentNumber;i++)
        if(curGMM->EnergyFG[i]==-1)
            curGMM->EnergyFG[i]=0;
    int betaNum=0;
    float meane1=0;
    float incR;
    betaMean = 0;
    for(int i=0;i<segmentNumber;i++)
     {
         meane1+=curGMM->EnergyBG[i];
         meane1+=curGMM->EnergyFG[i];

         for(int j = 0;j<(*regionNode)[i].neighborNum;j++)
         {

             incR = ((*regionNode)[i].meanR - (*regionNode)[(*regionNode)[i].neighborID[j]].meanR)*((*regionNode)[i].meanR - (*regionNode)[(*regionNode)[i].neighborID[j]].meanR);
             incR += ((*regionNode)[i].meanG - (*regionNode)[(*regionNode)[i].neighborID[j]].meanG)*((*regionNode)[i].meanG - (*regionNode)[(*regionNode)[i].neighborID[j]].meanG);
             incR += ((*regionNode)[i].meanB - (*regionNode)[(*regionNode)[i].neighborID[j]].meanB)*((*regionNode)[i].meanB - (*regionNode)[(*regionNode)[i].neighborID[j]].meanB);

            // incR = (*regionNode)[i].neighborDistance[j];
             betaMean += incR;
             betaNum++;
         }
     }
    betaMean = betaMean/betaNum;
    meane1=meane1/(2*segmentNumber);
    float lambda = LAMBDA;//If too low let changes on t-weights

    float e1const = E1CONST *(1/meane1);
    float e2const = E2CONST; //Less than 2

    float beta = 1/betaMean;
    float capp;
    float capp2;
    for(int i=0;i<segmentNumber;i++)
    {
          if((*regionNode)[i].groupID==1)
          {
             gr->add_tweights(i,0,100);//0.5 * 10 + 1
             oldIDs[i]=1;
          }
          else if((*regionNode)[i].groupID==3)
          {
             gr->add_tweights(i,100,0);
             oldIDs[i]=3;
          }
          else
              gr->add_tweights(i,e1const*curGMM->EnergyBG[i],e1const*curGMM->EnergyFG[i]);


          for(int j=0;j<(*regionNode)[i].neighborNum;j++)
          {
              capp = ((*regionNode)[i].meanR - (*regionNode)[(*regionNode)[i].neighborID[j]].meanR)*((*regionNode)[i].meanR - (*regionNode)[(*regionNode)[i].neighborID[j]].meanR);
              capp += ((*regionNode)[i].meanG - (*regionNode)[(*regionNode)[i].neighborID[j]].meanG)*((*regionNode)[i].meanG - (*regionNode)[(*regionNode)[i].neighborID[j]].meanG);
              capp += ((*regionNode)[i].meanB - (*regionNode)[(*regionNode)[i].neighborID[j]].meanB)*((*regionNode)[i].meanB - (*regionNode)[(*regionNode)[i].neighborID[j]].meanB);

              capp2 = capp;
              capp = exp((-1)*capp*beta*lambda);
              gr->add_edge(i,(*regionNode)[i].neighborID[j],e2const*capp,e2const*capp);

          }

    }
    gr->maxflow();
    for(int i=0;i<segmentNumber;i++)
        if(gr->what_segment(i)==FloatGraph::SOURCE)
            (*regionNode)[i].groupID = 2;
        else
            (*regionNode)[i].groupID = 1;

}


void GrabCut::runResidualGrabCutNoBG(){

    if(firstRun)
    {
        runResidualGrabCutFirstRun();
        firstRun=false;
        return;
    }

    float olde1const = e1const;
    EM* dummy;
    dummy = curGMM;
    curGMM = oldGMM;
    oldGMM = dummy;
    for(int i=0;i<segmentNumber;i++)
    {
        curGMM->RegionNumPcsdPixel[i]=0;
        oldGMM->RegionNumPcsdPixel[i]=0;
    }

    QColor c;
    for(int i=0;i<segmentNumber;i++)
            (*regionNode)[i].groupID = 3;

        for(int x=max(0,bBoxXmin-20);x<min(width,bBoxXmax+20);x++)
            for(int y=max(0,bBoxYmin-20);y<min(height,bBoxYmax+20);y++)
                (*regionNode)[segmentIDs[y*width+x]].groupID=2;

        int nID;
        for(int i=0;i<foregroundGraph.size();i++)
        {
            (*regionNode)[foregroundGraph[i]].groupID = 1;
            for(int j=0;j<(*regionNode)[foregroundGraph[i]].neighborNum;j++)
            {
                nID = (*regionNode)[foregroundGraph[i]].neighborID[j] ;
                (*regionNode)[nID].groupID=0;
            }
        }

        for(int i=0;i<foregroundGraph.size();i++)
            (*regionNode)[foregroundGraph[i]].groupID = 1;


        int rId;
        float inve[3];
        for(int w=0;w<width;w=w+6)
        {
            for(int h=0;h<height;h=h+6)
            {
                c = image->pixel(w,h);
                inve[0]=c.red();
                inve[1]=c.green();
                inve[2]=c.blue();
                rId = segmentIDs[w+h*width];
                curGMM->EnergyFG[rId]+=fgGMM.getLikelihood(inve);
                curGMM-> RegionNumPcsdPixel[rId]++;
            }
        }

        for(int i=0;i<segmentNumber;i++)
         {
             if(curGMM->RegionNumPcsdPixel[i]==0)
             {
                 curGMM->EnergyFG[i]=-1;
             }
             else
             {
                 curGMM->EnergyFG[i]=curGMM->EnergyFG[i]/((float)curGMM->RegionNumPcsdPixel[i]);
             }
         }

        int bCount =0;
        for(int w=0;w<width;w=w+16)
            for(int h=0;h<height;h=h+16){
                if((*regionNode)[ segmentIDs[h*width + w]].groupID==3){
                    c = image->pixel(w,h);
                    bgVec[bCount*3+0]= c.red();
                    bgVec[bCount*3+1]= c.green();
                    bgVec[bCount*3+2]= c.blue();
                    bCount++;
                }
            }

        for(int w=0;w<width;w=w+8)
            for(int h=0;h<height;h=h+8){
                if((*regionNode)[ segmentIDs[h*width + w]].groupID==2){
                    c = image->pixel(w,h);
                    bgVec[bCount*3+0]= c.red();
                    bgVec[bCount*3+1]= c.green();
                    bgVec[bCount*3+2]= c.blue();
                    bCount++;
                }
            }
        bgGMM.clear();
        bgGMM.insertData(bgVec,bCount);
        bgGMM.initLastMean();
        bgGMM.iterateGMM(3);

        for(int i=0;i<segmentNumber;i++)
             {
                 curGMM->RegionNumPcsdPixel[i]=0;
             }
            for(int w=0;w<width;w=w+6)
                for(int h=0;h<height;h=h+6){
                    c = image->pixel(w,h);
                    inve[0]=c.red();
                    inve[1]=c.green();
                    inve[2]=c.blue();
                    rId = segmentIDs[w+h*width];
                    curGMM->EnergyBG[rId]+=bgGMM.getLikelihood(inve);
                    curGMM->RegionNumPcsdPixel[rId]++;
                }

            for(int i=0;i<segmentNumber;i++)
             {
                 if(curGMM->RegionNumPcsdPixel[i]==0)
                     curGMM->EnergyBG[i]=0;
                 else
                     curGMM->EnergyBG[i]=curGMM->EnergyBG[i]/((float)curGMM->RegionNumPcsdPixel[i]);
             }
            for(int i=0;i<segmentNumber;i++)
                if(curGMM->EnergyFG[i]==-1)
                    curGMM->EnergyFG[i]=0;

            float meane1=0;
            float incR;
            for(int i=0;i<segmentNumber;i++)
             {
                 meane1+=curGMM->EnergyBG[i];
                 meane1+=curGMM->EnergyFG[i];

             }
            meane1=meane1/(2*segmentNumber);
            float lambda = LAMBDA;//If too low let changes on t-weights

            e1const = E1CONST *(1/meane1);
            float e2const = E2CONST; //Less than 2

            float beta = 1/betaMean;
            float capp;
            float capp2;
            for(int i=0;i<segmentNumber;i++)
            {
                  if((*regionNode)[i].groupID==1)
                  {
                      if(oldIDs[i]==1)
                      {
                          continue;
                      }
                      else
                      {
                          if(oldIDs[i]==0)
                              gr->set_trcap(i,gr->get_trcap(i)+(0-100-olde1const*oldGMM->EnergyBG[i]+olde1const*oldGMM->EnergyFG[i]));
                          else if(oldIDs[i]==3)
                              gr->set_trcap(i,gr->get_trcap(i)+(-200));
                          oldIDs[i]=1;
                          gr->mark_node(i);
                      }
                  }
                  else if((*regionNode)[i].groupID==3)
                  {
                      if(oldIDs[i]==3)
                      {
                          continue;
                      }
                      else
                      {
                          if(oldIDs[i]==0)
                              gr->set_trcap(i,gr->get_trcap(i)+(100-olde1const*oldGMM->EnergyBG[i]+olde1const*oldGMM->EnergyFG[i]));
                          else if(oldIDs[i]==1)
                              gr->set_trcap(i,gr->get_trcap(i)+(200));
                          oldIDs[i]=3;
                          gr->mark_node(i);
                      }
                  }
                  else
                  {
                      if(oldIDs[i]==1)
                      {
                          gr->set_trcap(i,gr->get_trcap(i)+(e1const*curGMM->EnergyBG[i]-e1const*curGMM->EnergyFG[i]+100));
                      }
                      else if(oldIDs[i]==0)
                      {
                          gr->set_trcap(i,gr->get_trcap(i)+(e1const*curGMM->EnergyBG[i]-e1const*curGMM->EnergyFG[i]-olde1const*oldGMM->EnergyBG[i]+olde1const*oldGMM->EnergyFG[i]));
                      }
                      else{
                          //3
                          gr->set_trcap(i,gr->get_trcap(i)+(e1const*curGMM->EnergyBG[i]-e1const*curGMM->EnergyFG[i]-100));
                      }
                      oldIDs[i]=0;
                      gr->mark_node(i);
                  }
            }
            gr->maxflow();
            for(int i=0;i<segmentNumber;i++)
                if(gr->what_segment(i)==FloatGraph::SOURCE)
                    (*regionNode)[i].groupID = 2;
                else
                    (*regionNode)[i].groupID = 1;

}


