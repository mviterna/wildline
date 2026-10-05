#pragma once
#include "CoreMinimal.h"
namespace WildlineAudio {
enum Event : uint8 { Rifle,Shotgun,Footstep,Pawstep,Jump,Land,Climb,Reload,Bolt,Pump,Pickup,Deposit,Checkpoint,Feeder,Hit,Death,Thermal,Bird,Cow,Horse,Win };
TArray<int16> Samples(Event Kind);
void Play(UObject* Context,Event Kind,FVector Position,bool Local=false);
}
