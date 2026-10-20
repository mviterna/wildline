#include "WildlineCharacter.h"
#include "WildlineWorld.h"
#include "WildlineGame.h"
#include "WildlineAudio.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"

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
    if(CourseCheckpoint==5){FeederVisits++;if(auto* PS=GetPlayerState<AWildlinePlayerState>()){PS->FeederScores++;PS->ForceNetUpdate();}if(auto* GM=GetWorld()->GetAuthGameMode<AWildlineGameMode>())GM->Deposit(1);CourseCheckpoint=0;CourseLane=-1;MulticastSound(WildlineAudio::Feeder);}
    else{CourseCheckpoint++;MulticastSound(WildlineAudio::Checkpoint);}
    ForceNetUpdate();
}

void AWildlineCharacter::CourseBotThink(float Delta){
    UpdateCourse();bSprinting=false;auto* M=GetCharacterMovement();
    if(BotCourseStep!=CourseCheckpoint){BotCourseStep=CourseCheckpoint;BotDecisionAt=ServerTime()+MatchRules().ReactionDelay(AIDifficulty==0?.8f:AIDifficulty==2?0.f:.2f);BotGoalUntil=ServerTime()+18;}
    // Retry a different authored route when the entry tree is congested or a
    // landing repeatedly fails. Each retry still starts at a real ground pad.
    if(CourseCheckpoint==1&&TreeIndex==INDEX_NONE&&!M->IsFalling()&&ServerTime()>BotGoalUntil){CourseCheckpoint=0;CourseLane=-1;BotCourseStep=-1;BotPatrolStep++;}
    const int32 Lane=CourseLane>=0?CourseLane:((FMath::Max(0,BotSlot)+BotPatrolStep)%2);
    FVector Target=Forest->CourseTarget(Lane,CourseCheckpoint);
    const int32 Entry=Forest->CourseEntryTree(Lane);
    if(Entry!=INDEX_NONE&&CourseCheckpoint==1){
        const auto& Tree=Forest->Trees[Entry];
        if(TreeIndex==Entry){
            if(GetActorLocation().Z<Tree.ClimbTop-35){ForwardInput=1;SideInput=0;return;}
            // The service wire leaves beneath a crossarm. Orbit to the clear
            // approach side before launching, including after a return trip.
            if((GetActorLocation()-Tree.Base).GetSafeNormal2D().X>-.85f){ForwardInput=0;SideInput=1;return;}
            ForwardInput=SideInput=0;if(Controller)Controller->SetControlRotation((Target-GetActorLocation()).GetSafeNormal2D().Rotation());ServerJump_Implementation();return;
        }
        if(!M->IsFalling()&&GetActorLocation().Z<Tree.ClimbTop-90){
            for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(*It!=this&&It->bSquirrel&&!It->bDead&&It->TreeIndex==Entry){
                // Wait clear of the trunk so the squirrel above can launch safely.
                BotProgressAt=ServerTime();M->StopMovementImmediately();return;
            }
            const FVector Toward=Tree.Base-GetActorLocation();
            if(Controller)Controller->SetControlRotation(Toward.GetSafeNormal2D().Rotation());
            if(Forest->NearestTree(GetActorLocation(),30)==Entry&&GetActorLocation().X<Tree.Base.X&&FMath::Abs(GetActorLocation().Y-Tree.Base.Y)<8)ServerClimb_Implementation();else {
                const FVector Grip=Tree.Base+FVector(-Tree.Radius-18,0,0);
                MoveBot(Grip,ServerTime(),Toward.Size2D()>250);AddMovementInput(BotMove,1);
            }return;
        }
    }
    // Return beside the course, rather than into the solid bins and bench legs.
    if(CourseCheckpoint==0&&GetActorLocation().X>-800){
        float Side=Forest->CourseTarget(Lane,0).Y;
        for(int32 Step=1;Step<=4;Step++)Side=Lane==0?FMath::Min(Side,Forest->CourseTarget(Lane,Step).Y):FMath::Max(Side,Forest->CourseTarget(Lane,Step).Y);
        Side+=Lane==0?-180.f:180.f;
        Target=FMath::Abs(GetActorLocation().Y-Side)>20?FVector(GetActorLocation().X,Side,210):FVector(-1050,Side,210);
    }
    // Leave the hopper through its open end before heading down the lawn.
    // The clear front/back panes are physical barriers above the seed tray.
    if(CourseCheckpoint==0&&FVector::Dist2D(GetActorLocation(),Forest->FeederLocation)<120&&GetActorLocation().Z>Forest->GroundHeight(GetActorLocation().X,GetActorLocation().Y)+30)Target=Forest->FeederLocation+FVector(-160,0,-240);
    const FVector D=Target-GetActorLocation();const FVector Dir=D.GetSafeNormal2D();
    if(Controller)Controller->SetControlRotation(Dir.Rotation());
    if(M->IsFalling()){
        // Braking is necessary: a full long jump otherwise overshoots the narrow props.
        const float Distance=D.Size2D();const float Speed=M->Velocity.Size2D();
        if(Distance<1)M->Velocity.X=M->Velocity.Y=0;
        else{
            const float Approach=FMath::Min(FMath::Min(FMath::Max(Speed,80.f),600.f),Distance/FMath::Max(Delta,.001f));
            M->Velocity.X=Dir.X*Approach;M->Velocity.Y=Dir.Y*Approach;
        }
        return;
    }
    // Keep airborne steering continuous; difficulty adds a pause only on landing.
    if(ServerTime()<BotDecisionAt){M->StopMovementImmediately();return;}
    if(CourseCheckpoint==0){
        if(GetActorLocation().Z>Forest->GroundHeight(GetActorLocation().X,GetActorLocation().Y)+30){AddMovementInput(Dir,1);return;}
        if(D.Size2D()>25){MoveBot(Target,ServerTime(),D.Size2D()>150);AddMovementInput(BotMove,1);}return;
    }
    const float Height=D.Z+13; // surface height above the squirrel's feet
    const float LaunchZ=Forest->CourseLaunchZ(GetActorLocation());
    if(Height>LaunchZ*LaunchZ/(2*980.f*M->GravityScale)+5){CourseCheckpoint=0;CourseLane=-1;return;}
    if(Stamina<22){M->StopMovementImmediately();return;}
    const float Gravity=980.f*M->GravityScale;
    const float Flight=(LaunchZ+FMath::Sqrt(FMath::Max(0.f,LaunchZ*LaunchZ-2*Gravity*Height)))/Gravity;
    if(D.Size2D()>FMath::Max(30.f,600.f*Flight-10)){AddMovementInput(Dir,1);return;}
    ServerJump_Implementation();
}
