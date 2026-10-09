#pragma once
#include "CoreMinimal.h"
class USoundWave;
namespace WildlineAudio {
enum Event : uint8 { Rifle,Shotgun,Footstep,Pawstep,Jump,Land,Climb,Reload,Bolt,Pump,Pickup,Deposit,Checkpoint,Feeder,Hit,Death,Thermal,Bird,Cow,Horse,Win };
TArray<int16> Samples(Event Kind);
USoundWave* FootstepSound(int32 Variant);
constexpr float RunningFootstepRate=1.1f;
void Play(UObject* Context,Event Kind,FVector Position,bool Local=false,float PlaybackRate=1.f);
}
