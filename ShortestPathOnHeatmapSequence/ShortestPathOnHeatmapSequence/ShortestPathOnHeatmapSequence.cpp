// ShortestPathOnHeatmapSequence.cpp : メイン プロジェクト ファイルです。

#include "windows.h"
#include "MyForm.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem> 
#include "ShortestPathForHeatMap.h"

using namespace System;
using namespace System::IO;
using namespace System::Runtime::InteropServices;

using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;



static void n_marshalString(String ^ s, std::string& os) {
  const char* chars = (const char*)(Marshal::StringToHGlobalAnsi(s)).ToPointer();
  os = chars;
  Marshal::FreeHGlobal(IntPtr((void*)chars));
}


class LessString_ForFnameTime {
public:
  bool operator()
    (
    const std::pair<std::string,std::string>& rsLeft, 
    const std::pair<std::string,std::string>& rsRight
    ) const 
  {
    if(rsLeft.first.length() == rsRight.first.length()) return rsLeft.first < rsRight.first;
    else                                                return rsLeft.first.length() < rsRight.first.length();
  }
};






static bool t_showOpenFileDlg_multi
(
  const char* filter,
  std::vector<std::string> &fNames
)
{
  OpenFileDialog^ dlg = gcnew OpenFileDialog();
  dlg->Filter = gcnew System::String(filter);
  dlg->Multiselect = true;

  
  if (dlg->ShowDialog() == System::Windows::Forms::DialogResult::Cancel) return false;


  std::vector<std::pair<std::string, std::string>> vec_fname_date;

  for each (auto it in  dlg->FileNames)
  {
    std::string fName, fTime;
    n_marshalString(it, fName);
    n_marshalString(File::GetCreationTime(it).ToString(), fTime);
    vec_fname_date.push_back(make_pair(fName, fTime));
  }

  //sort fname_ftime 
  //currentry the system only uses file name
  std::sort(vec_fname_date.begin(), vec_fname_date.end(), LessString_ForFnameTime() );
  
  fNames.clear(); 
  for( const auto &it : vec_fname_date) fNames.push_back(it.first);

  return true;
}




static bool t_showSaveFileDlg
(
  const char *filter, 
  std::string &fname
)
{
  SaveFileDialog^ dlg = gcnew SaveFileDialog();
  dlg->Filter = gcnew System::String(filter);

  if (dlg->ShowDialog() == System::Windows::Forms::DialogResult::Cancel) return false;

  IntPtr mptr = Marshal::StringToHGlobalAnsi(dlg->FileName);
  fname = static_cast<const char*>(mptr.ToPointer());
  return true;
}







static bool isJpgFile(const std::string &fname)
{
  int flen = (int)fname.length();
  if ( flen < 4 ) return false;
  std::string ex = fname.substr(flen-4, flen );
  
  return strcmp( ex.c_str(), ".png") == 0 || 
         strcmp( ex.c_str(), ".PNG") == 0 ||
         strcmp( ex.c_str(), ".Png") == 0;

}


static std::vector<std::string> getAllPngFileInDir( std::string _path )
{
  std::vector<std::string> child_img_files;
  std::tr2::sys::path parent_path( _path ); 

  for(std::tr2::sys::path p : std::tr2::sys::directory_iterator(parent_path))
  {
    if ( std::tr2::sys::is_regular_file(p) && isJpgFile(p.filename().string() ) )
    { 
      child_img_files.push_back(_path + "/" + p.filename().string());
    } 
  }
  return child_img_files;
}







// Export key points
// nose, leye, reye, lear, rear, lShoulder, rshoulder, leftElbow, rightElbow, 
// leftWrist, rightWrist, leftHip, rightHip, leftKnee, rightKnee, leftAnkle, rightAnkle
//








[STAThreadAttribute]
int main(array<System::String ^> ^args)
{
  std::cout << "------------------------ USEAGE ----------------------" << std::endl;
  std::cout << "$ShortestPathOnHeatmap.exe  dir_name  csv_file_name   " << std::endl;
  std::cout << "or " << std::endl;
  std::cout << "set heatmap_Path and csv_filename by Dialog" << std::endl;
  std::cout << "コンソールからヒートマップが入ったフォルダパスと出力ファイル名を指定して起動できます．" << std::endl;
  std::cout << "または，起動後にダイアログより入力・出力ファイルを指定できます．" << std::endl;
    

  
  //1. get input_files_names and output_file_path 
  std::vector< std::string > input_heatmap_paths;
  std::string output_csv_path;

  if( args->Length >=2 )
  {
	  IntPtr mptr1 = Marshal::StringToHGlobalAnsi(args[0]);
    std::string dir_path = static_cast<const char*>(mptr1.ToPointer());
    input_heatmap_paths = getAllPngFileInDir(dir_path.c_str());

	  IntPtr mptr2 = Marshal::StringToHGlobalAnsi(args[1]);
    output_csv_path = static_cast<const char*>(mptr2.ToPointer());
  }
  if( input_heatmap_paths.size() == 0 ){
    if( !t_showOpenFileDlg_multi("heatmap sequence (*.png)|*.png", input_heatmap_paths) ) return 0;   
    if( !t_showSaveFileDlg      ("landmark sequence(*.csv)|*.csv", output_csv_path    ) ) return 0;
  }
  
  std::cout << "file size  " << input_heatmap_paths.size() << "\n";
  

  //2. compute shortest path
  std::vector< std::vector<Point2i> > points_sequence;
  computeLandmarkSequenceByHeatmaps(input_heatmap_paths, output_csv_path, points_sequence);
  
  return 0;
}
