#include "ShortestPathForHeatMap.h"
#include "timageloader.h"
#include <algorithm>



#define IMG_WIDTH    200
#define IMG_HEIGHT   200
#define KEYPOINT_NUM 25


#ifndef byte
typedef unsigned char byte;
#endif


static int WIN_R = 5;
static int WIN_W = 2 * WIN_R + 1;


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
  
  int   winR  = WIN_R;
  int   winW  = WIN_W;
  float alpha = 1.0f;

  for( int y = 0; y < H; ++y) for( int x = 0; x < W; ++x)
  {
    int   maxFrom = -1;
    float maxV    =  0;
    for( int yy = -winR; yy <= winR; ++yy ) if( 0 <= y+yy && y+yy < H)  
    {
      for( int xx = -winR; xx <= winR; ++xx ) if( 0 <= x+xx && x+xx < W)
      {
        float e = engyPre[(x+xx) + (y+yy)*W] - alpha * sqrt(xx*xx + yy*yy);
        if( e > maxV) {
          maxV    = e;
          maxFrom = xx + winR + (yy + winR) * winW;
        }
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
  const int frameN = endI - startI -1; 


  //0. 必要なフィールドを用意（fromが大きい）
  byte  **heatMaps    = new byte* [KEYPOINT_NUM];
  float **engyMaps    = new float*[KEYPOINT_NUM];
  float **engyMapsPre = new float*[KEYPOINT_NUM];
  
  for( int i=0; i < KEYPOINT_NUM; ++i){
    heatMaps   [i] = new byte [W*H];
    engyMaps   [i] = new float[W*H];    
    engyMapsPre[i] = new float[W*H];
    memset(engyMaps   [i], 0, sizeof(float) *W*H);    
    memset(engyMapsPre[i], 0, sizeof(float) *W*H);    
  }
  std::vector<byte**> fromMaps(frameN, 0);
  for( int f = 0; f < frameN; ++f)
  {
    fromMaps[f] = new byte*[KEYPOINT_NUM];
    for( int i=0; i < KEYPOINT_NUM; ++i) fromMaps[f][i] = new byte [W*H];
  }
  

  //1. 0フレーム目の取得と初期化
  t_loadOpenPoseHeatmaps(fnames[startI].c_str(), W,H,KEYPOINT_NUM, heatMaps);
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


  //2. DPで shortest path計算 
  for( int f = startI + 1; f < endI; ++f) 
  {
    t_loadOpenPoseHeatmaps(fnames[f].c_str(), W, H, KEYPOINT_NUM, heatMaps);

    for( int k=0; k < KEYPOINT_NUM; ++k)
    {
      byte*  heat = heatMaps   [k];
      byte*  from = fromMaps[f][k];
      float* engy = engyMaps   [k];
      float* engyP= engyMapsPre[k];
      t_stepDP( W, H, heat, engyP, engy, from);
      memcpy( engyP, engy, sizeof(float) * W * H);   
    }
  }
  
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
      int  fromY = fromI / WIN_W - WIN_W;
      xy[0] += fromX;
      xy[1] += fromY;
    }
  }


  //後片付け
  for( int i=0; i < KEYPOINT_NUM; ++i){
    delete[] heatMaps  [i];
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
