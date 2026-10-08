#include "WildlineCharacter.h"
#include "WildlineWorld.h"
#include "WildlineGame.h"
#include "WildlineAudio.h"
#include "GameFramework/CharacterMovementComponent.h"

void AWildlineCharacter::UpdateCourse(){
    const auto* S=GetWorld()->GetGameState<AWildlineGameState>();
    if(bInLobby||!HasAuthority()||!Forest||!Forest->bResidential||!bSquirrel||bDead||TreeIndex!=INDEX_NONE||!S||!S->bRoundStarted||S->Winner)return;
    auto* Move=GetCharacterMovement();if(!Move->IsMovingOnGround())return;
    // A run begins on a ground-level start pad. Every elevated checkpoint requires
    // a real landing. Staying at the feeder never produces additional points.
    if(CourseCheckpoint==0){
        for(int32 Lane=0;Lane<2;Lane++)if(Forest->OnCourseTarget(GetActorLocation(),Lane,0)){CourseLane=Lane;CourseCheckpoint=1;MulticastSound(WildlineAudio::Checkpoint);break;}
        return;
    }
    const float Height=GetActorLocation().Z-Forest->GroundHeight(GetActorLocation().X,GetActorLocation().Y);
    if(CourseCheckpoint>1&&Height<25){CourseCheckpoint=0;CourseLane=-1;return;}
    if(!Forest->OnCourseTarget(GetActorLocation(),CourseLane,CourseCheckpoint))return;
    // Also check the support point, so a checkpoint cannot be credited through a
    // wall, on a roof, or while the capsule is near it at the wrong elevation.
    FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(CourseLanding),false,this);
    const FVector Target=Forest->CourseTarget(CourseLane,CourseCheckpoint);
    if(!GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation(),GetActorLocation()-FVector(0,0,25),ECC_Visibility,Q)||FMath::Abs(Hit.ImpactPoint.Z-Target.Z)>3)return;
    if(CourseCheckpoint==5){FeederVisits++;if(auto* GM=GetWorld()->GetAuthGameMode<AWildlineGameMode>())GM->Deposit(1);CourseCheckpoint=0;CourseLane=-1;MulticastSound(WildlineAudio::Feeder);}
    else{CourseCheckpoint++;MulticastSound(WildlineAudio::Checkpoint);}
    ForceNetUpdate();
}

void AWildlineCharacter::CourseBotThink(float Delta){
    UpdateCourse();bSprinting=false;auto* M=GetCharacterMovement();
    const int32 Lane=CourseLane>=0?CourseLane:(GetUniqueID()%2);
    FVector Target=Forest->CourseTarget(Lane,CourseCheckpoint);
    // Return beside the course, rather than into the solid bins and bench legs.
    if(CourseCheckpoint==0&&GetActorLocation().X>-800){const float Side=Lane==0?-500.f:500.f;Target=FMath::Abs(GetActorLocation().Y-Side)>40?FVector(GetActorLocation().X,Side,210):FVector(-1050,Side,210);}
    const FVector D=Target-GetActorLocation();const FVector Dir=D.GetSafeNormal2D();
    if(Controller)Controller->SetControlRotation(Dir.Rotation());
    if(M->IsFalling()){
        // Braking is necessary: a full long jump otherwise overshoots the narrow props.
        const float Distance=D.Size2D();const float Speed=M->Velocity.Size2D();
        if(Distance<40||FVector::DotProduct(M->Velocity,Dir)<0)M->Velocity.X=M->Velocity.Y=0;
        else if(Speed>0){M->Velocity.X=Dir.X*FMath::Min(Speed,600.f);M->Velocity.Y=Dir.Y*FMath::Min(Speed,600.f);}
        return;
    }
    // Keep airborne steering continuous; difficulty adds a pause only on landing.
    if(BotCourseStep!=CourseCheckpoint){BotCourseStep=CourseCheckpoint;BotDecisionAt=ServerTime()+(AIDifficulty==0?.8f:AIDifficulty==2?0.f:.2f);}
    if(ServerTime()<BotDecisionAt){M->StopMovementImmediately();return;}
    if(CourseCheckpoint==0){
        if(GetActorLocation().Z>Forest->GroundHeight(GetActorLocation().X,GetActorLocation().Y)+30){AddMovementInput(Dir,1);return;}
        if(D.Size2D()>25)AddMovementInput(Dir,1);return;
    }
    const float Height=D.Z+13; // surface height above the squirrel's feet
    if(Height>65){CourseCheckpoint=0;CourseLane=-1;return;}
    if(Stamina<22){M->StopMovementImmediately();return;}
    if(D.Size2D()>300){AddMovementInput(Dir,1);return;}
    ServerJump_Implementation();
}
