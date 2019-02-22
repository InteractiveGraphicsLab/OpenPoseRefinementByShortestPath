#include "ShortestPathForHeatMap.h"
#include "timageloader.h"

#include <time.h>
#include <iostream>
#include <algorithm>
using namespace std;


#define IMG_WIDTH    656
#define IMG_HEIGHT   368
#define KEYPOINT_NUM 25


#ifndef byte
typedef unsigned char byte;
#endif


#define WIN_R 5
#define WIN_W 11

static float ENERGY_MAP[ WIN_W * WIN_W ];



void t_stepDP
(
  const int W,
  const int H,
  const byte  *heat,
  const float *engyPre,
  float *engyNew,
  byte  *from 
)
{

  float alpha = 1.0f;

  for( int y = 0; y < H; ++y) 
  for( int x = 0; x < W; ++x)
  {
    int sYY = (y - WIN_R >= 0 ) ? - WIN_R : - y;
    int sXX = (x - WIN_R >= 0 ) ? - WIN_R : - x;
    int eYY = (y + WIN_R <  H ) ?   WIN_R : H-1-y;
    int eXX = (x + WIN_R <  W ) ?   WIN_R : W-1-x;

    int   maxFrom = -1;
    float maxV    = -FLT_MAX;
    for( int yy = sYY; yy <= eYY; ++yy ) 
    for( int xx = sXX; xx <= eXX; ++xx ) 
    {
      //float e = (float)( engyPre[(x+xx) + (y+yy)*W] - alpha * sqrt(xx*xx + yy*yy) ) ;
      float e = (float)( engyPre[(x+xx) + (y+yy)*W] - alpha * ENERGY_MAP[xx + WIN_R + WIN_W * (yy+WIN_R)] );
      if( e > maxV) {
        maxV    = e;
        maxFrom = xx + WIN_R + (yy + WIN_R) * WIN_W;
      }
    }
    
    engyNew[x + y*W] = maxV + heat[ x + y * W];
    from   [x + y*W] = maxFrom;

  }
}





Point2i t_getMaxXY(const int W, const int H, const float *enegymap)
{
  float maxV = -100000;
  Point2i xy; 
  for( int y=0; y < H; ++y){
    for( int x=0; x < W; ++x){
      if( enegymap[x+y*W] > maxV ) {
        maxV = enegymap[x+y*W];
        xy.data[0] = x;
        xy.data[1] = y;
      }
    }
  }
  cout << "t_getMAXXY  " << maxV << " " << xy[0] << " " << xy[1] << " " << endl;
  return xy;
}



void t_computeLandmarkSequenceByHeatMap 
  ( 
    const std::vector<std::string> &fnames,
    const int startI,
    const int endI  ,
    std::vector<std::vector<Point2i>>  &points_seq // [frameIdx][landmarkId]            
  )
{
  //constant values
  const int W  = IMG_WIDTH ;
  const int H  = IMG_HEIGHT;
  const int WH = W * H;
  const int frameN = endI - startI; 


  //0. 必要なフィールドを用意（fromが大きい）
  byte  **heatMaps    = new byte* [KEYPOINT_NUM];
  float **engyMaps    = new float*[KEYPOINT_NUM];
  float **engyMapsPre = new float*[KEYPOINT_NUM];
  
  for( int i=0; i < KEYPOINT_NUM; ++i){
    heatMaps   [i] = new byte [WH];
    engyMaps   [i] = new float[WH];    
    engyMapsPre[i] = new float[WH];
    memset(heatMaps   [i], 0, sizeof(byte)  *WH);    
    memset(engyMaps   [i], 0, sizeof(float) *WH);    
    memset(engyMapsPre[i], 0, sizeof(float) *WH);    
  }

  std::vector<byte**> fromMaps(frameN, 0);
  for( int f = 0; f < frameN; ++f)
  {
    fromMaps[f] = new byte*[KEYPOINT_NUM];
    for( int i=0; i < KEYPOINT_NUM; ++i) {
      fromMaps[f][i] = new byte [WH];
      memset( fromMaps[f][i], 0, sizeof(byte) * WH);
    }
  }
  
  //1. 0フレーム目の取得と初期化
  t_loadOpenPoseHeatmaps(fnames[startI].c_str(), W, H, KEYPOINT_NUM, heatMaps);

  for( int k=0; k < KEYPOINT_NUM; ++k)
  {
    byte*  heat = heatMaps   [k];
    byte*  from = fromMaps[0][k];
    float* engy = engyMaps   [k];
    float* engyP= engyMapsPre[k];
    for( int i=0; i < WH; ++i) {
      from[i] = 0;
      engy[i] = engyP[i] = heat[i];
    }
  }  

  int time1 = 0, time2 = 0;

  //2. DPで shortest path計算 
  for( int f = startI + 1; f < endI; ++f) 
  {
    time_t t0 = clock(); 
    t_loadOpenPoseHeatmaps(fnames[f].c_str(), W, H, KEYPOINT_NUM, heatMaps);
    cout << endl << "img" << f <<  endI << (int)fnames.size() << frameN << endl;

    time_t t1 = clock(); 

#pragma omp parallel for
    for( int k=0; k < KEYPOINT_NUM; ++k)
    {
      byte*  heat  = heatMaps   [k];
      byte*  from  = fromMaps[f][k];
      float* engy  = engyMaps   [k];
      float* engyP = engyMapsPre[k];
      t_stepDP( W, H, heat, engyP, engy, from);
      memcpy( engyP, engy, sizeof(float) * W * H);   
      cout << k ;
    }
    time_t t2 = clock(); 
    time1 += (int)( t1-t0 );
    time2 += (int)( t2-t1 );
  } 
  

  cout << endl;
  cout << "time " << time1 / (double) CLOCKS_PER_SEC << " " << time2 / (double) CLOCKS_PER_SEC << endl; 
  cout << endl << "compute path" << endl;
  
  //3. path計算
  points_seq.clear();
  points_seq.resize(frameN, std::vector<Point2i>(KEYPOINT_NUM, Point2i()) );
  for( int k=0; k < KEYPOINT_NUM; ++k)
  {
    //3a 開始点get
    Point2i xy = t_getMaxXY( W,H, engyMaps[k]);
    
    for( int f = frameN-1; f >= 0; --f)
    {
      //3b 値を入れる
      points_seq[f][k] = xy;
      //3c xyの更新
      int  fromI = fromMaps[f][k][ xy[0] + xy[1] * W ];
      int  fromX = fromI % WIN_W - WIN_R;
      int  fromY = fromI / WIN_W - WIN_R;
      xy[0] += fromX;
      xy[1] += fromY;
      
      cout << fromX << " " << fromY << " ";
      cout << xy[0] << " " << xy[1] << endl;
    }
  }
  cout << "path computation done" << endl;


  //後片付け
  for( int i=0; i < KEYPOINT_NUM; ++i){
    delete[] heatMaps   [i];
    delete[] engyMaps   [i];
    delete[] engyMapsPre[i];
  }
  delete[] heatMaps  ;     
  delete[] engyMaps   ;    
  delete[] engyMapsPre;    

  for( int f = 0; f < frameN; ++f)
  {
    for( int i=0; i < KEYPOINT_NUM; ++i) delete[] fromMaps[f][i];
    delete[] fromMaps[f];
  }
}



bool computeLandmarkSequenceByHeatmaps
(
    const std::vector<std::string> &fnames,
    std::vector<std::vector<Point2i>>  &points_sequence // [frameIdx][landmarkId]            
)
{

  static bool isFirst = true;
  if ( isFirst ) {
    for( int x = 0; x < WIN_W; ++x) {
      for( int y = 0; y < WIN_W; ++y) {
        ENERGY_MAP[x + y*WIN_W ] = (float) sqrt( (x - WIN_R ) * (x - WIN_R ) + (y - WIN_R ) * (y - WIN_R ) );
        cout << ENERGY_MAP[x + y*WIN_W ] << " ";
      }
      cout << endl;
    }
    isFirst = false;    
  }





  const int SEQUENCE_BUNCH_NUM = 1000; //600 * 800 * 25 * 1000 
  
  points_sequence.clear();

  for( int i=0; i < (int)fnames.size(); i += SEQUENCE_BUNCH_NUM)
  {
    const int startI = i;
    const int endI   = std::min( i + SEQUENCE_BUNCH_NUM , (int)fnames.size() );
    std::vector<std::vector<Point2i>> points_seq;
    t_computeLandmarkSequenceByHeatMap ( fnames, startI, endI, points_seq);

    for( const auto &it : points_seq) points_sequence.push_back(it);
  }
  
  return true;
}
