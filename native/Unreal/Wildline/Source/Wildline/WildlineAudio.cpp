#include "WildlineAudio.h"
#include "WildlineUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"
#include "Sound/SoundAttenuation.h"
#include "Engine/World.h"
#include "Components/AudioComponent.h"
#include "TimerManager.h"

TArray<int16> WildlineAudio::Samples(Event Kind){
    const float Duration=Kind==Win?1.25f:Kind==Reload?1.1f:Kind==Cow||Kind==Horse?.75f:Kind==Bird?.48f:Kind==Death?.5f:Kind==Rifle||Kind==Shotgun?.75f:Kind==Feeder||Kind==Deposit?.6f:.23f;
    TArray<int16> PCM;PCM.SetNum(FMath::CeilToInt(Duration*22050));FRandomStream R(371+Kind);float Filter=0;
    for(int32 I=0;I<PCM.Num();I++){
        const float T=I/22050.f,N=R.FRandRange(-1,1);Filter=Filter*.83f+N*.17f;
        auto Tone=[&](float F){return FMath::Sin(2*PI*F*T);};
        auto Pulse=[&](float At,float Width){return T>=At?FMath::Exp(-(T-At)/Width):0.f;};
        float V=0;
        switch(Kind){
        case Rifle:case Shotgun:V=(N*.58f+Filter*.7f+Tone(Kind==Rifle?83:57)*.42f)*Pulse(0,.055f)+(Filter*.4f+Tone(41)*.08f)*Pulse(.045f,.20f);break;
        case Footstep:case Pawstep:break; // Recorded waves are played below.
        case Jump:V=Filter*.20f*Pulse(0,.065f)+Tone(750-T*1800)*.055f*FMath::Sin(PI*T/Duration);break;
        case Land:V=(Filter*.7f+Tone(65)*.18f)*Pulse(0,.055f);break;
        case Climb:V=N*.09f*(Pulse(0,.018f)+Pulse(.085f,.027f));break;
        case Reload:V=(N*.20f+Tone(1700)*.10f)*(Pulse(.02f,.018f)+Pulse(.32f,.022f)+Pulse(.70f,.025f))+Filter*.17f*Pulse(.10f,.15f);break;
        case Bolt:case Pump:V=(N*.23f+Tone(Kind==Bolt?2100:900)*.15f)*(Pulse(.015f,.012f)+Pulse(.12f,.02f));break;
        case Pickup:case Checkpoint:V=(Tone(Kind==Pickup?950:720)+Tone(Kind==Pickup?1425:1080)*.35f)*.13f*Pulse(0,.07f);break;
        case Deposit:case Feeder:case Win:{const int32 Note=FMath::Min(Kind==Win?3:2,int32(T/(Kind==Win?.27f:.16f)));const float Notes[]={523.25f,659.25f,783.99f,1046.5f};const float Age=T-Note*(Kind==Win?.27f:.16f);V=Tone(Notes[Note])*.18f*FMath::Exp(-Age/.12f);break;}
        case Hit:V=(Filter*.45f+N*.12f)*Pulse(0,.045f);break;
        case Death:V=(Tone(140-T*140)*.10f+Filter*.13f)*Pulse(0,.15f);break;
        case Thermal:V=Tone(1400)*.09f*(Pulse(0,.018f)+Pulse(.10f,.018f));break;
        case Cow:case Horse:{const float F=(Kind==Horse?280:105)+FMath::Sin(T*9)*30;V=(Tone(F)+.28f*Tone(F*2))*FMath::Sin(PI*T/.75f)*.20f;break;}
        case Bird:V=FMath::Sin(2*PI*(2400*T+40*FMath::Sin(T*38)))*.045f*FMath::Pow(FMath::Max(0.f,FMath::Sin(T*28)),3.f);break;
        }
        const float Fade=FMath::Clamp((Duration-T)*100.f,0.f,1.f)*FMath::Clamp(T*1800,0.f,1.f);
        PCM[I]=static_cast<int16>(FMath::Clamp(V*Fade,-.95f,.95f)*32767.f);
    }
    return PCM;
}
USoundWave* WildlineAudio::FootstepSound(int32 Variant){
    const FString Name=FString::Printf(TEXT("SW_Footstep%02d"),FMath::Clamp(Variant,0,3)+1);
    return LoadObject<USoundWave>(nullptr,*FString::Printf(TEXT("/Game/Wildline/Audio/%s.%s"),*Name,*Name));
}
void WildlineAudio::Play(UObject* Context,Event Kind,FVector Position,bool Local,float PlaybackRate){
    if(!Context||!Context->GetWorld()||Context->GetWorld()->GetNetMode()==NM_DedicatedServer)return;
    const auto* Settings=UWildlineUserSettings::Get();const float Volume=Settings?Settings->SoundVolume:.8f;if(Volume<=0)return;
    USoundWave* Sound=nullptr;const bool RecordedStep=Kind==Footstep||Kind==Pawstep;
    if(RecordedStep){
        Sound=FootstepSound(FMath::RandRange(0,3));
        if(!Sound){UE_LOG(LogTemp,Error,TEXT("WILDLINE missing recorded footstep wave"));return;}
    }else{
        auto* Stream=NewObject<USoundWaveProcedural>(Context);Stream->SetSampleRate(22050);Stream->NumChannels=1;
        const auto PCM=Samples(Kind);Stream->Duration=PCM.Num()/22050.f;Stream->bLooping=false;
        Stream->QueueAudio(reinterpret_cast<const uint8*>(PCM.GetData()),PCM.Num()*sizeof(int16));Sound=Stream;
    }
    PlaybackRate=FMath::Clamp(PlaybackRate,.5f,2.f);
    UAudioComponent* Component=nullptr;
    if(Local)Component=UGameplayStatics::SpawnSound2D(Context,Sound,Volume,PlaybackRate);
    else{
    auto* Attenuation=NewObject<USoundAttenuation>(Context);auto& A=Attenuation->Attenuation;
    A.bAttenuate=true;A.bSpatialize=true;A.AttenuationShape=EAttenuationShape::Sphere;A.AttenuationShapeExtents=FVector(100);
    A.FalloffDistance=Kind==Rifle||Kind==Shotgun?14000.f:Kind==Bird?3000.f:1800.f;
    Component=UGameplayStatics::SpawnSoundAtLocation(Context,Sound,Position,FRotator::ZeroRotator,Volume,PlaybackRate,0.f,Attenuation);}
    // Procedural streams otherwise continue submitting silence after their PCM ends.
    if(Component&&!RecordedStep){FTimerHandle Stop;Context->GetWorld()->GetTimerManager().SetTimer(Stop,FTimerDelegate::CreateWeakLambda(Component,[Component](){Component->FadeOut(.025f,0.f);}),Sound->Duration/PlaybackRate+.15f,false);}
}
