#include "WildlineUserSettings.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

static FString ProfileFixtureRoot(){FString Dir;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Dir);return Dir;}

FString UWildlineUserSettings::LocalProfilePath(){
    // QA fixtures stay isolated. Normal packages and the installed beta share
    // one profile even when the package directory changes after a build/update.
    const FString Custom=ProfileFixtureRoot();
    return FPaths::Combine(Custom.IsEmpty()?FPaths::Combine(FPlatformProcess::UserSettingsDir(),TEXT("WILDLINE")):Custom,TEXT("Profiles/LocalPlayer.ini"));
}
bool UWildlineUserSettings::SaveLocalProfile() const{
    const FString Path=LocalProfilePath();IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
    FConfigFile File;File.SetString(TEXT("LocalPlayer"),TEXT("Name"),*CleanPlayerName(PlayerDisplayName));
    FString Look;FWildlineHunterProfile::StaticStruct()->ExportText(Look,&HunterProfile,nullptr,nullptr,PPF_None,nullptr);
    File.SetString(TEXT("LocalPlayer"),TEXT("Hunter"),*Look);File.SetString(TEXT("LocalPlayer"),TEXT("Version"),TEXT("1"));
    const FString Temp=Path+TEXT(".tmp");
    return File.Write(Temp)&&IFileManager::Get().Move(*Path,*Temp,true,true);
}
void UWildlineUserSettings::LoadLocalProfile(){
    FConfigFile File;const FString Path=LocalProfilePath();
    if(FPaths::FileExists(Path)){File.Read(Path);
        FString Name,Look;if(File.GetString(TEXT("LocalPlayer"),TEXT("Name"),Name)&&!CleanPlayerName(Name).IsEmpty())PlayerDisplayName=CleanPlayerName(Name);
        if(File.GetString(TEXT("LocalPlayer"),TEXT("Hunter"),Look)){FWildlineHunterProfile Loaded;if(FWildlineHunterProfile::StaticStruct()->ImportText(*Look,&Loaded,nullptr,PPF_None,GWarn,TEXT("Local hunter"))){Loaded.Clamp();HunterProfile=Loaded;}}
        return;
    }
    if(!PlayerDisplayName.IsEmpty()||!ProfileFixtureRoot().IsEmpty())return;
    // Recover the most recently used real package, never arbitrary QA saves.
    TArray<FString> Candidates;Candidates.Add(FPaths::Combine(FPlatformProcess::UserSettingsDir(),TEXT("Wildline/Saved/Config/Windows/GameUserSettings.ini")));
    const FString Builds=FPaths::ConvertRelativePathToFull(FPaths::RootDir()/TEXT("../.."));
    if(FPaths::FileExists(Builds/TEXT("../LaunchNative.ps1"))){
        Candidates.Add(Builds/TEXT("Windows/Wildline/Saved/Config/Windows/GameUserSettings.ini"));
        TArray<FString> Folders;IFileManager::Get().FindFiles(Folders,*(Builds/TEXT("*")),false,true);
        for(const auto& Folder:Folders)Candidates.Add(Builds/Folder/TEXT("Windows/Wildline/Saved/Config/Windows/GameUserSettings.ini"));
    }
    FDateTime Newest=FDateTime::MinValue();
    for(const FString& Candidate:Candidates){FConfigFile Legacy;FString Name;Legacy.Read(Candidate);if(Legacy.GetString(TEXT("/Script/Wildline.WildlineUserSettings"),TEXT("PlayerDisplayName"),Name)&&!CleanPlayerName(Name).IsEmpty()&&IFileManager::Get().GetTimeStamp(*Candidate)>Newest){
        PlayerDisplayName=CleanPlayerName(Name);Newest=IFileManager::Get().GetTimeStamp(*Candidate);
        FString Look;if(Legacy.GetString(TEXT("/Script/Wildline.WildlineUserSettings"),TEXT("HunterProfile"),Look)){FWildlineHunterProfile Loaded;if(FWildlineHunterProfile::StaticStruct()->ImportText(*Look,&Loaded,nullptr,PPF_None,GWarn,TEXT("Migrated hunter"))){Loaded.Clamp();HunterProfile=Loaded;}}
    }}
}
