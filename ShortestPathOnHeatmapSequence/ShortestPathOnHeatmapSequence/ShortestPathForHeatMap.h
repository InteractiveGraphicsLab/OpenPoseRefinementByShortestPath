#pragma once

#include <vector>
#include <string>


class Point2i
{
public:
  int data[2];

  Point2i(){
    data[0] = data[1] = 0;
  }

  //vectorに入れるのでコピーコンストラクタとoperator=は必須
  Point2i(const Point2i &src){
    copy(src);
  }
  Point2i &operator=( const Point2i &src )
  {
    copy( src );
    return *this;
  }
  
  void copy( const Point2i &src){
    data[0] = src.data[0];
    data[1] = src.data[1];
  }
  
  //便利アクセスツール
  inline int& operator[](int i) { return data[i]; }  
};



bool computeLandmarkSequenceByHeatmaps
(
    const std::vector<std::string> &fnames,
    const std::string output_file_path,
    std::vector<std::vector<Point2i>>  &points_sequence // [frameIdx][landmarkId]            
);

