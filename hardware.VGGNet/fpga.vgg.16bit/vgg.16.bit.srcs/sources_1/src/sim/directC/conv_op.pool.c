// ---------------------------------------------------
// File       : conv_op.pool.c
//
// Description: generate procRam, calculate convolution continuously
//
// Version    : 1.0
// ---------------------------------------------------
#include<stdio.h>
#include<stdlib.h>
#include"DirectC.h"
#include<cblas.h>

#define data_height 16
#define data_width 16
#define patch_height 14
#define patch_width 14
#define ker_height 3
#define ker_width 3
#define ker_channels 32

union val{
  float fVal;
  unsigned char ucVal[sizeof(float)];
  struct dataEntry{
    unsigned int ithFM:8;
    unsigned int barIdx:8;
    unsigned int ithAtom:8;
    unsigned int atomEntry:8;
  } entryVal;
};
// -------------------------------- directC function -------------------------------- 
const int atomicHeight      = 14;
const int atomicWidth       = 14;
const int patchHeight       = 16;
const int patchWidth        = 16;
const int microPatchHeight  = 14;
const int microPatchWidth   = 7;
const int micro3PatchSize   = 14*7*3; // size of 3 micro-patch
const int micro2PatchSize   = 14*7*2; // size of 2 micro-patch
const int micro1PatchSize   = 14*7*1; // size of 1 micro-patch
const int procRamWidth  = 3*7 + 1; // processing ram width
const int procRamHeight = 14+1; // processing ram height - 1 pel (top row)
const int ddrDataWidth  = 64;
const int floatNumWidth = 32;

// open file with file name
void* getFileDescriptor(char *fileName)
{
  FILE *fd = NULL;
  printf("file name: %s\n", fileName);
  fd = fopen(fileName, "r");
  if(fd <= 0){
    printf("could not open file: %s\n", fileName);
  } else {
    printf("%s opened\n", fileName);
  }
  return((void*)fd);
}

// close opened file
void closeFile(void *fileDescriptor)
{
  int status;
  status = fclose((FILE*) fileDescriptor);
  if(status == 0) {
    printf("file colsed with no error\n");
  } else {
    printf("file colsed with ERROR\n");
  }
  return;
}

// reading control
void readControl(U resetDone, U convLastPos, U* readBottomData, U* bottomDataAddr,
                 U xEndPos, U yEndPos, U* xPos, U* yPos, U* isFirstFM, U* ithOffset,
                 U* barOffset, U* ithFM, U* readEnd, U* readKer, U* readKerAddr, U* ithKerSet)
{
  const int channels  = 3; // number of bottom channels
  const int fmSize    = 25088; // fmHeight*fmWidth*floatNumWidth/ddrDataWidth;
  const int kerSize   = 144;   // kerChannels*kerWidth*kerHeight*floatNumWidth/ddrDataWidth;
  const int numKerSet = 2; // number of ker_set
  *barOffset  = 1568; // fmWidth*atomicHeight*floatNumWidth/ddrDataWidth;
  if(resetDone) {
  // initialization
    *xPos = 0;
    *yPos = 0;
    *ithOffset = 0;
    *ithFM     = 0;
    // read kernel
    *ithKerSet = 0;
    *readKerAddr  = 0;
    *readKer= 1;
    *isFirstFM = 1;
    *readBottomData = 1;
    printf("reset done\n");
    printf("reading info: ");
    printf("resetDone readBottomData: %8x, ", *readBottomData);
    printf("readBottomData: %8x, ", *readBottomData);
    printf("bottomDataAddr: %8x, ", *bottomDataAddr);
    printf("xEndPos: %8x, ", xEndPos);
    printf("yEndPos: %8x,\n", yEndPos);
    printf("\txPos: %8x, ", *xPos);
    printf("yPos: %8x, ", *yPos);
    printf("isFirstFM: %8x, ", *isFirstFM);
    printf("ithOffset: %8x, ", *ithOffset);
    printf("barOffset: %8x, ", *barOffset);
    printf("ithFM: %8x\n", *ithFM);
    printf("\treadKer: %8x, ", *readKer);
    printf("ithKerSet: %8x, ", *ithKerSet);
    printf("readKerAddr: %8x, ", *readKerAddr);
    printf("readEnd: %8x\n", *readEnd);
  } else if(convLastPos) {
    *isFirstFM  = 0;
    *readEnd    = 0;
    *readKer    = 0;
    *readBottomData = 0;
  // convolution at last pos
    if(*ithKerSet < (numKerSet-1)) {
      *ithKerSet += 1;
      *readKer = 1;
      *readKerAddr += kerSize;
    } else {
      *ithKerSet    = 0;
      *readKer      = 1;
      *readKerAddr  = 0;
    }

    if(*ithKerSet == 0){
      if(*ithFM==(channels-1)){
        // xPos
        if(*xPos<xEndPos)
          *xPos += 1;
        else
          *xPos  = 0;
        // yPos
        if(*xPos == 0){
          if(*yPos < yEndPos)
            *yPos += 1;
          else{
            *yPos  = 0;
            *readEnd = 1;
          }
        }
        // ith feature map
        *ithFM = 0;
        *ithOffset = 0;
        *isFirstFM = 1;
      } else {
        *ithFM      += 1;
        *ithOffset  += fmSize;
        *isFirstFM   = 0;
      }
      *readBottomData = 1;
    }
    printf("convLastPos\n");
    printf("reading info: ");
    printf("convLastPos readBottomData: %8x, ", *readBottomData);
    printf("readBottomData: %8x, ", *readBottomData);
    printf("bottomDataAddr: %8x, ", *bottomDataAddr);
    printf("xEndPos: %8x, ", xEndPos);
    printf("yEndPos: %8x,\n", yEndPos);
    printf("\txPos: %8x, ", *xPos);
    printf("yPos: %8x, ", *yPos);
    printf("isFirstFM: %8x, ", *isFirstFM);
    printf("ithOffset: %8x, ", *ithOffset);
    printf("barOffset: %8x, ", *barOffset);
    printf("\tithFM: %8x\n", *ithFM);
    printf("readKer: %8x, ", *readKer);
    printf("ithKerSet: %8x, ", *ithKerSet);
    printf("readKerAddr: %8x, ", *readKerAddr);
    printf("readEnd: %8x\n", *readEnd);
  }
  return;
}

// generate procRam
void readProcRam(void *fileDescriptor, U readBottomData, U xPos, U yPos, U xEndPos, U yEndPos, U barOffset, U ithFM, U* RAM, U* procRamFull)
{
  union val procRAM[16*16] = {0}; // processing memory
  unsigned int atomEntry = 0;
  int bTL=0, bTR=0, bTopPanel=0, bLeftPanel=0, bRightPanel=0, bBottomPanel=0, bBL=0, bBR=0;

  if(readBottomData){
    if(yPos==0){
      if(xPos==0){
        bRightPanel  = 1;
        bBottomPanel = 1;
        bBR = 1;
      }else if(xPos==xEndPos){
        bLeftPanel   = 1;
        bBottomPanel = 1;
        bBL = 1;
      }else{
        bLeftPanel   = 1;
        bRightPanel  = 1;
        bBottomPanel = 1;
        bBL = 1;
        bBR = 1;
      }
    }else if(yPos==yEndPos){
      if(xPos==0){
        bTopPanel   = 1;
        bRightPanel = 1;
        bTR = 1;
      }else if(xPos==xEndPos){
        bTopPanel   = 1;
        bLeftPanel  = 1;
        bTL = 1;
      }else{
        bLeftPanel  = 1;
        bRightPanel = 1;
        bTopPanel   = 1;
        bTL = 1;
        bTR = 1;
      }
    }else{
      if(xPos==0){
        bTopPanel   = 1;
        bBottomPanel = 1;
        bRightPanel  = 1;
        bTR = 1;
        bBR = 1;
      }else if(xPos==xEndPos){
        bTopPanel   = 1;
        bBottomPanel = 1;
        bLeftPanel   = 1;
        bTL = 1;
        bBL = 1;
      }else{
        bTopPanel   = 1;
        bBottomPanel = 1;
        bLeftPanel   = 1;
        bRightPanel  = 1;
        bTL = 1;
        bTR = 1;
        bBL = 1;
        bBR = 1;
      }
    }
  }
  {
  // top left corner
  if(bTL) {
    procRAM[0].entryVal.ithFM   = ithFM;
    procRAM[0].entryVal.barIdx  = yPos-1;
    procRAM[0].entryVal.ithAtom = xPos-1;
    procRAM[0].entryVal.atomEntry = 49*4-1;
  } else {
    procRAM[0].fVal = 0.;
  }
  // top right corner
  if(bTR) {
    procRAM[0].entryVal.ithFM   = ithFM;
    procRAM[0].entryVal.barIdx  = yPos-1;
    procRAM[0].entryVal.ithAtom = xPos+1;
    procRAM[0].entryVal.atomEntry = 49*3-7;
  } else {
    procRAM[0].fVal = 0.;
  }
  // top panel
  if(bTopPanel){
    atomEntry = 49*3-7;
    for(int i=1; i<patchWidth/2; i++){
      procRAM[i].entryVal.ithFM  = ithFM;
      procRAM[i].entryVal.barIdx = yPos-1;
      procRAM[i].entryVal.ithAtom = xPos;
      procRAM[i].entryVal.atomEntry = atomEntry;
      atomEntry += 1;
    }
    atomEntry += 49-7;
    for(int i=1; i<patchWidth/2; i++){
      procRAM[i].entryVal.ithFM  = ithFM;
      procRAM[i].entryVal.barIdx = yPos-1;
      procRAM[i].entryVal.ithAtom = xPos;
      procRAM[i].entryVal.atomEntry = atomEntry;
      atomEntry += 1;
    }
  }else{
    for(int i=1; i<patchWidth-1; i++)
      procRAM[i].fVal = 0.;
  }
  // left panel
  if(bLeftPanel){
    atomEntry = 49+7-1;
    for(int i=1; i<patchHeight/2; i++){
      procRAM[i*patchWidth].entryVal.ithFM  = ithFM;
      procRAM[i*patchWidth].entryVal.barIdx = yPos;
      procRAM[i*patchWidth].entryVal.ithAtom = xPos-1;
      procRAM[i*patchWidth].entryVal.atomEntry = atomEntry;
      atomEntry += 7;
    }
    atomEntry += 49;
    for(int i=patchHeight/2; i<patchHeight-1; i++){
      procRAM[i*patchWidth].entryVal.ithFM  = ithFM;
      procRAM[i*patchWidth].entryVal.barIdx = yPos;
      procRAM[i*patchWidth].entryVal.ithAtom = xPos-1;
      procRAM[i*patchWidth].entryVal.atomEntry = atomEntry;
      atomEntry += 7;
    }
  } else {
    for(int i=1; i<patchHeight-1; i++)
      procRAM[i*patchWidth].fVal = 0.;
  }
  // right panel
  if(bRightPanel){
    atomEntry = 0;
    for(int i=1; i<patchHeight/2; i++){
      procRAM[i*patchWidth + patchWidth-1].entryVal.ithFM  = ithFM;
      procRAM[i*patchWidth + patchWidth-1].entryVal.barIdx = yPos;
      procRAM[i*patchWidth + patchWidth-1].entryVal.ithAtom = xPos+1;
      procRAM[i*patchWidth + patchWidth-1].entryVal.atomEntry = atomEntry;
      atomEntry += 7;
    }
    atomEntry += 49;
    for(int i=patchHeight/2; i<patchHeight-1; i++){
      procRAM[i*patchWidth + patchWidth-1].entryVal.ithFM  = ithFM;
      procRAM[i*patchWidth + patchWidth-1].entryVal.barIdx = yPos;
      procRAM[i*patchWidth + patchWidth-1].entryVal.ithAtom = xPos+1;
      procRAM[i*patchWidth + patchWidth-1].entryVal.atomEntry = atomEntry;
      atomEntry += 7;
    }
  } else {
    for(int i=1; i<patchHeight-1; i++)
      procRAM[i*patchWidth + patchWidth-1].fVal = 0.;
  }
  // bottom panel
  if(bBottomPanel){
    atomEntry = 0;
    for(int i=1; i<patchWidth/2; i++){
      procRAM[patchWidth*(patchHeight-1)+i].entryVal.ithFM  = ithFM;
      procRAM[patchWidth*(patchHeight-1)+i].entryVal.barIdx = yPos+1;
      procRAM[patchWidth*(patchHeight-1)+i].entryVal.ithAtom = xPos;
      procRAM[patchWidth*(patchHeight-1)+i].entryVal.atomEntry = atomEntry;
      atomEntry += 1;
    }
    atomEntry += 49-7;
    for(int i=patchWidth/2; i<patchWidth-1; i++){
      procRAM[patchWidth*(patchHeight-1)+i].entryVal.ithFM  = ithFM;
      procRAM[patchWidth*(patchHeight-1)+i].entryVal.barIdx = yPos+1;
      procRAM[patchWidth*(patchHeight-1)+i].entryVal.ithAtom = xPos;
      procRAM[patchWidth*(patchHeight-1)+i].entryVal.atomEntry = atomEntry;
      atomEntry += 1;
    }
  }else{
    for(int i=1; i<patchWidth-1; i++)
      procRAM[patchWidth*(patchHeight-1)+i].fVal = 0.;
  }
  // bottom left corner
  if(bBL) {
    procRAM[patchWidth*(patchHeight-1)].entryVal.ithFM  = ithFM;
    procRAM[patchWidth*(patchHeight-1)].entryVal.barIdx = yPos+1;
    procRAM[patchWidth*(patchHeight-1)].entryVal.ithAtom= xPos-1;
    procRAM[patchWidth*(patchHeight-1)].entryVal.atomEntry = 56;
  }else{
    procRAM[patchWidth*(patchHeight-1)].fVal = 0.;
  }
  // bottom right corner
  if(bBR) {
    procRAM[patchHeight*patchWidth-1].entryVal.ithFM  = ithFM;
    procRAM[patchHeight*patchWidth-1].entryVal.barIdx = yPos+1;
    procRAM[patchHeight*patchWidth-1].entryVal.ithAtom= xPos+1;
    procRAM[patchHeight*patchWidth-1].entryVal.atomEntry = 0;
  }else{
    procRAM[patchHeight*patchWidth-1].fVal = 0.;
  }
  }
  // central 14x14
  atomEntry = 0;
  for(int r=1; r<patchHeight-1; r++){
    for(int c=1; c<patchWidth/2; c++){
      procRAM[r*patchHeight + c].entryVal.ithFM = ithFM;
      procRAM[r*patchHeight + c].entryVal.barIdx= yPos;
      procRAM[r*patchHeight + c].entryVal.ithAtom   = xPos;
      procRAM[r*patchHeight + c].entryVal.atomEntry = atomEntry;
      ++atomEntry;
    }
    for(int c=patchWidth/2; c<patchWidth-1; c++){
      procRAM[r*patchHeight + c].entryVal.ithFM = ithFM;
      procRAM[r*patchHeight + c].entryVal.barIdx= yPos;
      procRAM[r*patchHeight + c].entryVal.ithAtom   = xPos;
      procRAM[r*patchHeight + c].entryVal.atomEntry = atomEntry;
      ++atomEntry;
    }
  }
    // check
    union val *p = procRAM;
    for(int r=0; r<16; r++){
      for(int c=0; c<16; c++){
        for(int i=0; i<sizeof(union val); i++)
          printf("%.2x",p->ucVal[sizeof(union val)-1-i]);
        printf("_");
        ++p;
      }
      printf("\n");
    }
  memcpy((void*)RAM, (void*)procRAM, (sizeof(union val)*16*16));
  *procRamFull = 1;
  return;
}

void readProcKer(void *fileDescriptor, U readKerData, U readKerAddr, U ithKerSet, U* kerRam, U* kerRamFull)
{
  long lOffset;
  const int iKerHeight = 3;
  const int iKerWidth  = 3;
  const int iKerChannels = 32;
  const long lKerSetSize = iKerChannels*iKerHeight*iKerWidth;
  lOffset = ithKerSet*lKerSetSize*sizeof(union val);
  printf("ker lOffset: %8x\n", lOffset);
  fseek(fileDescriptor, lOffset, SEEK_SET);
  fread(kerRam, 1, lKerSetSize*sizeof(union val), (FILE*) fileDescriptor);
  *kerRamFull = 1;

//// check
//union val *p;
//p=(union val*)kerRam;
//for(int i=0; i<32*9/16; i++) {
//  for(int j=0; j<16; j++){
//    for(int k=0; k<sizeof(union val); k++)
//      printf("%.2x", p->ucVal[sizeof(union val)-1-k]);
//    printf("_");
//    ++p;
//  }
//  printf("\n");
//}

  return;
}

float* reformBottomData(const U* bottomData)
{
  float *p=NULL, *tmp=NULL;
  float *pData=NULL;
  const int kerHeight=3, kerWidth=3, kerChannels=32;
  const int dataHeight = 16, dataWidth=16;
  p=(float*)malloc(atomicHeight*atomicWidth*kerHeight*kerWidth*sizeof(union val));
  pData = (float*)bottomData;
  tmp = p;
  for(int kerRow=0; kerRow<kerHeight; kerRow++){
    for(int kerCol=0; kerCol<kerWidth; kerCol++){
      for(int patchCol=0; patchCol<atomicWidth; patchCol++){
        for(int patchRow=0; patchRow<atomicHeight; patchRow++){
        //*(tmp+kerRow*kerWidth+kerCol + (patchCol*atomicHeight+patchRow)*kerHeight*kerWidth)
        //  = *(pData+kerRow*atomicWidth+kerCol + patchRow*atomicWidth+patchCol);
          memcpy((tmp+kerRow*kerWidth+kerCol + (patchCol*atomicHeight+patchRow)*kerHeight*kerWidth), (pData+kerRow*dataWidth+kerCol + patchRow*dataWidth+patchCol), sizeof(union val));
        }
      }
    }
  }

//// check
//union val* pCheck = (union val*)tmp;
//for(int r=0; r<atomicHeight*atomicWidth; r++){
//  for(int c=0; c<kerHeight*kerWidth; c++){
//    for(int k=0; k<sizeof(union val); k++)
//      printf("%.2x", (pCheck+c+kerHeight*kerWidth*r)->ucVal[sizeof(union val)-1-k]);
//    printf("_");
//  }
//  printf("\n");
//}
//pCheck=(union val*) bottomData;
//for(int r=0; r<16; r++){
//  for(int c=0; c<16; c++){
//    for(int i=0; i<sizeof(union val); i++)
//      printf("%.2x", pCheck->ucVal[sizeof(union val)-1-i]);
//    printf("_");
//    ++pCheck;
//  }
//  printf("\n");
//}

  return(p);
}

float* reformKerData(U* kerData)
{
  float* p=NULL;
  const int kerHeight=3, kerWidth=3, kerChannels=32;
  p=(float*)malloc(sizeof(union val)*kerChannels*kerHeight*kerWidth);
  memcpy((void*)p, (void*)kerData, sizeof(union val)*kerChannels*kerHeight*kerWidth);
  return(p);
}

void cnnConv(U startConv, U* convOutput, U* bottomData, U* kerData)
{
  if(startConv){
    const int kerHeight=3, kerWidth=3, kerChannels=32;
    const int dataHeight = 16, dataWidth=16;
    float *convPatch=NULL;
    float *convKer=NULL;
    float *convResult= (float*)malloc(kerChannels*atomicHeight*atomicWidth*sizeof(union val));
    convPatch = reformBottomData(bottomData);
    convKer   = reformKerData(kerData);

    // correlation
    int iLDA=kerHeight*kerWidth;
    int iLDB=kerHeight*kerWidth;
    int iLDC=kerChannels;
    float fAlpha=1.0, fBeta=0.;
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans, atomicHeight*atomicWidth, kerChannels, kerHeight*kerWidth, fAlpha, convPatch, iLDA, convKer, iLDB, fBeta, convResult, iLDC);
    free(convPatch);
    free(convKer);
    free(convResult);
    convPatch =NULL;
    convKer   =NULL;
    convResult=NULL;

  //// check
  //union val* p=convOutput;
  //for(int i=0; i<dataHeight*dataWidth; i++){
  //  for(int j=0; j<kerHeight*kerWidth; j++){
  //  }
  //}
  }

  return;
}

/*
//void cmpFloatNum(U dataValid, U numOfValidData, U* data, U* RAM, U cnt)
U cmpFloatNum(U dataValid, U numOfValidData, U* data, U* RAM, U cnt, U xPos, U yPos, U ithFM)
// U cnt -- current position on RAM
{
  U checkPass = 1;
  U i=0;
  union val *pData  = data;
  union val *pRAM   = RAM;
  pRAM += cnt;
  if(dataValid) {
    for(i=0; i<numOfValidData; i++){
      for(int s=0; s<sizeof(union val); s++) {
        if(pData->ucVal[s] != pRAM->ucVal[s]) {
          printf("pData: %2x, pRAM: %2x\n", pData->ucVal[s], pRAM->ucVal[s]);
          checkPass = 0;
          printf("ERROR on cnt: %d, ", cnt);
          printf("xPos: %8x, ", xPos);
          printf("yPos: %8x, ", yPos);
          printf("ithFM: %8x\n", ithFM);
        //exit(1);
        }
      }
    //printf("data:%8x, RAM: %8x\n", *pData, *pRAM);
    //printf("checked\n");
      ++pData;
      ++pRAM;
    }
  }
  return checkPass;
}
 */
