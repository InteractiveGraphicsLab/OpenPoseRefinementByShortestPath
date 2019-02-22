#pragma once

#include <iostream>


#pragma managed


inline bool t_loadOpenPoseHeatmaps
(
    const char *fname,
    int W       ,
    int H       ,
    int heatmapN,
    unsigned char **heatmaps //allocated
)
{
  System::String ^s            = gcnew System::String(fname);
  System::Drawing::Image^ img  = System::Drawing::Image::FromFile(s);
  System::Drawing::Bitmap^ bmp = gcnew System::Drawing::Bitmap(img);

  if (bmp->Width != W * heatmapN || bmp->Height != H ) return false;
  
  System::Drawing::Imaging::BitmapData ^bmpData =
    bmp->LockBits(System::Drawing::Rectangle(0, 0, W, H),
      System::Drawing::Imaging::ImageLockMode::WriteOnly,
      bmp->PixelFormat);

  unsigned char* pBuf = (unsigned char*)bmpData->Scan0.ToPointer();

  //‹P“x’l‚ÌŽæ“¾AÝ’è 
  if ( bmp->PixelFormat == System::Drawing::Imaging::PixelFormat::Format8bppIndexed )
  {
    std::cout << "8 bit image" << std::endl;

    for (int y = 0; y < H; y++)
    {
      for ( int hmI = 0; hmI < heatmapN; ++hmI )
      {
        for (int x = 0; x < W; x++)
        {
          const int I = x + y*W;
          heatmaps[hmI][I] = pBuf[ (hmI * W + x) + y * bmpData->Stride];
        }
      } 
    }
  }
  else if ( bmp->PixelFormat == System::Drawing::Imaging::PixelFormat::Format24bppRgb ||
            bmp->PixelFormat == System::Drawing::Imaging::PixelFormat::Format32bppArgb)
  {
    std::cout << "24 bit image" << std::endl;
    //24bit BGRBGRBGR...   32bit BGRA BGRABGRA...
    int BitCount = bmp->GetPixelFormatSize(bmp->PixelFormat);
    int Step = BitCount / 8;

    for (int y = 0; y < H; y++)
    {
      for ( int hmI = 0; hmI < heatmapN; ++hmI )
      {
        for (int x = 0; x < W; x++)
        {
          const int I = x + y*W;
          heatmaps[hmI][I] = pBuf[ (hmI * W + x) * Step + y * bmpData->Stride]; //B 
        }
      }
    }
  }
  else
  {
    std::cout << "-------------------------error unknown format" << std::endl;
    return false;
  }

  bmp->UnlockBits(bmpData);
  img = nullptr;
  bmp = nullptr;
  return true;
}






inline void t_saveImage(
  const char *fname,
  const int &W,
  const int &H,
  const unsigned char* rgba)
{

  System::Drawing::Bitmap^ bmp = gcnew System::Drawing::Bitmap(W, H, System::Drawing::Imaging::PixelFormat::Format32bppRgb);

  System::Drawing::Imaging::BitmapData ^bmpData = bmp->LockBits(
      System::Drawing::Rectangle(0, 0, W, H),
      System::Drawing::Imaging::ImageLockMode::WriteOnly,
      bmp->PixelFormat);

  unsigned char* pBuf = (unsigned char*)bmpData->Scan0.ToPointer();


  //32bit BGRA BGRABGRA...

  int BitNum = bmp->GetPixelFormatSize(bmp->PixelFormat);
  int Step = BitNum / 8;

  for (int y = 0; y < H; y++)
  {
    for (int x = 0; x < W; x++)
    {
      const int I = 4 * (x + y*W);
      pBuf[x * Step + 0 + y * bmpData->Stride] = rgba[I + 2]; //B 
      pBuf[x * Step + 1 + y * bmpData->Stride] = rgba[I + 1]; //G 
      pBuf[x * Step + 2 + y * bmpData->Stride] = rgba[I + 0]; //R
    }
  }

  System::String ^fname_str = gcnew System::String(fname);
  System::String ^ext = System::IO::Path::GetExtension(fname_str)->ToLower();

  if (ext == ".bmp") bmp->Save(fname_str, System::Drawing::Imaging::ImageFormat::Bmp);
  else if (ext == ".jpg") bmp->Save(fname_str, System::Drawing::Imaging::ImageFormat::Jpeg);
  else if (ext == ".png") bmp->Save(fname_str, System::Drawing::Imaging::ImageFormat::Png);
  else if (ext == ".tif") bmp->Save(fname_str, System::Drawing::Imaging::ImageFormat::Tiff);
}


inline void t_saveImage_gray(
  const char *fname,
  const int &W,
  const int &H,
  const unsigned char* img)
{

  System::Drawing::Bitmap^ bmp = gcnew System::Drawing::Bitmap(W, H, System::Drawing::Imaging::PixelFormat::Format32bppRgb);

  System::Drawing::Imaging::BitmapData ^bmpData = bmp->LockBits(
      System::Drawing::Rectangle(0, 0, W, H),
      System::Drawing::Imaging::ImageLockMode::WriteOnly,
      bmp->PixelFormat);

  unsigned char* pBuf = (unsigned char*)bmpData->Scan0.ToPointer();


  //32bit BGRA BGRABGRA...

  int BitNum = bmp->GetPixelFormatSize(bmp->PixelFormat);
  int Step = BitNum / 8;

  for (int y = 0; y < H; y++)
  {
    for (int x = 0; x < W; x++)
    {
      const int I = x + y*W;
      pBuf[x * Step + 0 + y * bmpData->Stride] = img[I]; //B 
      pBuf[x * Step + 1 + y * bmpData->Stride] = img[I]; //G 
      pBuf[x * Step + 2 + y * bmpData->Stride] = img[I]; //R
    }
  }

  System::String ^fname_str = gcnew System::String(fname);
  System::String ^ext = System::IO::Path::GetExtension(fname_str)->ToLower();

  if (ext == ".bmp") bmp->Save(fname_str, System::Drawing::Imaging::ImageFormat::Bmp);
  else if (ext == ".jpg") bmp->Save(fname_str, System::Drawing::Imaging::ImageFormat::Jpeg);
  else if (ext == ".png") bmp->Save(fname_str, System::Drawing::Imaging::ImageFormat::Png);
  else if (ext == ".tif") bmp->Save(fname_str, System::Drawing::Imaging::ImageFormat::Tiff);
}
