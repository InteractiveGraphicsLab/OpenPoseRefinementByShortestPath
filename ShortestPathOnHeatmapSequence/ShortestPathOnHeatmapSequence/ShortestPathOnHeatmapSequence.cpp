// ShortestPathOnHeatmapSequence.cpp : メイン プロジェクト ファイルです。

#include "windows.h"
#include "MyForm.h"
#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>


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

  printf("aaa");

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



[STAThreadAttribute]
int main(array<System::String ^> ^args)
{
  printf("start shortest path on heat map sequene\n");
  
  //1. get input_files_names and output_file_path 
  std::vector< std::string > input_heatmap_paths;
  std::string output_csv_path;
  
  if( !t_showOpenFileDlg_multi("heatmap sequence (*.png)|*.png", input_heatmap_paths) ) return 0;   
  if( !t_showSaveFileDlg      ("landmark sequence(*.csv)|*.csv", output_csv_path    ) ) return 0;   

  //2. compute shortest path
    

  //3. output csv file


  return 0;
}
