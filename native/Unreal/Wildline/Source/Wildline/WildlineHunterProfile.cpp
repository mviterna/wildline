#include "WildlineHunterProfile.h"
#include "ImageUtils.h"
#include "ImageCore.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/Base64.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Modules/ModuleManager.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <commdlg.h>
#include "Windows/HideWindowsPlatformTypes.h"
#undef LoadImage
#endif
FString WildlinePortrait::SavedPhotoPath(){return FPaths::ProjectSavedDir()/TEXT("Profiles/HunterFace.jpg");}
static bool ReadPortrait(const FString& Encoded,FImage& Image){
    if(Encoded.IsEmpty()||Encoded.Len()>48000)return false;
    TArray<uint8> Data;if(!FBase64::Decode(Encoded,Data))return false;
    auto& Module=FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
    const EImageFormat Format=Module.DetectImageFormat(Data.GetData(),Data.Num());if(Format!=EImageFormat::JPEG)return false;
    auto Wrapper=Module.CreateImageWrapper(Format);
    if(!Wrapper||!Wrapper->SetCompressed(Data.GetData(),Data.Num())||Wrapper->GetWidth()<16||Wrapper->GetHeight()<16||Wrapper->GetWidth()>512||Wrapper->GetHeight()>512)return false;
    return FImageUtils::DecompressImage(Data.GetData(),Data.Num(),Image);
}
bool WildlinePortrait::ValidatePhoto(const FString& Encoded){FImage I;return ReadPortrait(Encoded,I);}
UTexture2D* WildlinePortrait::DecodePhoto(const FString& Encoded){FImage I;return ReadPortrait(Encoded,I)?FImageUtils::CreateTexture2DFromImage(I):nullptr;}
FString WildlinePortrait::LoadSavedPhoto(){TArray<uint8> Bytes;return FFileHelper::LoadFileToArray(Bytes,*SavedPhotoPath())?FBase64::Encode(Bytes):FString();}
bool WildlinePortrait::ImportPhoto(const FString& Path,FString& Encoded,FString& Error){
    const int64 Size=IFileManager::Get().FileSize(*Path);
    if(Size<1||Size>12*1024*1024){Error=TEXT("Choose a PNG or JPEG under 12 MB.");return false;}
    const FString Ext=FPaths::GetExtension(Path).ToLower();if(Ext!=TEXT("png")&&Ext!=TEXT("jpg")&&Ext!=TEXT("jpeg")){Error=TEXT("This image format is unsupported. Choose PNG or JPEG.");return false;}
    FImage Image;if(!FImageUtils::LoadImage(*Path,Image)||Image.SizeX<64||Image.SizeY<64||Image.SizeX>8192||Image.SizeY>8192){Error=TEXT("Could not read this image. Use a portrait between 64 and 8192 pixels.");return false;}
    FImage Small;const float Scale=512.f/FMath::Max(Image.SizeX,Image.SizeY);Image.ResizeTo(Small,FMath::Max(16,FMath::RoundToInt(Image.SizeX*Scale)),FMath::Max(16,FMath::RoundToInt(Image.SizeY*Scale)),ERawImageFormat::BGRA8,EGammaSpace::sRGB);
    TArray64<uint8> Bytes;for(int32 Quality:{85,75,65,55}){Bytes.Reset();if(FImageUtils::CompressImage(Bytes,TEXT("jpg"),Small,Quality)&&Bytes.Num()<=35000)break;}if(Bytes.Num()==0||Bytes.Num()>35000){Error=TEXT("Could not prepare this portrait. Try a simpler crop.");return false;}
    Encoded=FBase64::Encode(Bytes.GetData(),Bytes.Num());
    return true;
}
FString WildlinePortrait::ChoosePhotoFile(){
#if PLATFORM_WINDOWS
    wchar_t File[32768]={};OPENFILENAMEW Dialog={};Dialog.lStructSize=sizeof(Dialog);Dialog.lpstrFile=File;Dialog.nMaxFile=32768;Dialog.lpstrFilter=L"Portrait images (*.png;*.jpg;*.jpeg)\0*.png;*.jpg;*.jpeg\0\0";Dialog.lpstrTitle=L"Choose a front-facing hunter portrait";Dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
    if(GetOpenFileNameW(&Dialog))return FString(File);
#endif
    return FString();
}
