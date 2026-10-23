#include "WildlineCharacter.h"
#include "WildlineWorld.h"
#include "WildlineLake.h"
#include "WildlineGame.h"
#include "WildlineRules.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"

void AWildlineCharacter::ResetBotAwareness(){
    BotTarget.Reset();BotLastSeenAt=-100;BotDecisionAt=BotShotAt=0;BotCourseStep=-1;bAiming=false;bSprinting=false;
    BotFoodIndex=BotEscapeTree=BotTrackedTree=INDEX_NONE;BotPatrolStep=0;BotBlockedFood.Empty();
    BotPlanAt=BotGoalUntil=BotEscapeUntil=BotTreeAt=BotTreeCooldown=0;
    BotProgressAt=ServerTime();BotProgressPosition=GetActorLocation();BotMove=FVector::ZeroVector;
    BotProgressGoal=GetActorLocation();BotClosestGoalDistance=MAX_flt;BotGoalProgressAt=ServerTime();
    BotSteerSide=BotSlot%2==0?1.f:-1.f;ForwardInput=SideInput=0;
    if(auto* AI=Cast<AAIController>(Controller))AI->ClearFocus(EAIFocusPriority::Gameplay);
}

bool AWildlineCharacter::BotCanSee(const AWildlineCharacter* Other,float Range)const{
    if(!Other||Other->bDead||Other->bInLobby||Other->bPreviewHunter||Other==this)return false;
    const float Distance=FVector::Dist(GetActorLocation(),Other->GetActorLocation());
    if(Distance>Range||(Other->bSquirrel&&Forest->InCropCover(Other->GetActorLocation())&&!Other->bSprinting&&Distance>900))return false;
    const FVector Eye=GetActorLocation()+FVector(0,0,bSquirrel?6:72*HunterProfile.HeightScale());
    FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(BotSight),false,this);
    return !GetWorld()->LineTraceSingleByChannel(Hit,Eye,Other->GetActorLocation()+FVector(0,0,5),ECC_Visibility,Query)||Hit.GetActor()==Other;
}

FVector AWildlineCharacter::BotSteering(FVector Target,bool bSeparate)const{
    const FVector Position=GetActorLocation();FVector Desired=(Target-Position).GetSafeNormal2D();
    if(Desired.IsNearlyZero())return Desired;
    if(bSeparate){
        FVector Separation=FVector::ZeroVector;float Space=MatchRules().BotSpacing;
        // The porch is a shared narrow passage, so single-file spacing takes over there.
        if(bSquirrel&&Forest->bFarm&&FMath::Abs(Position.X-(Forest->FarmhouseLocation.X-720))<480&&FMath::Abs(Position.Y)<230)Space=FMath::Min(Space,35.f);
        for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It){
            if(*It==this||It->bDead||It->bInLobby||It->bPreviewHunter||It->bSquirrel!=bSquirrel||FMath::Abs(It->GetActorLocation().Z-Position.Z)>90)continue;
            const FVector Away=Position-It->GetActorLocation();const float Distance=Away.Size2D();
            if(Distance<Space){
                // A stable slot direction breaks symmetry even when two spawns coincide.
                const FVector Direction=Distance>1?Away.GetSafeNormal2D():FVector(0,BotSlot<It->BotSlot?-1:1,0);
                Separation+=Direction*(1-Distance/Space);
            }
        }
        Desired=(Desired+Separation*1.8f).GetSafeNormal2D();
    }
    FCollisionQueryParams Query(SCENE_QUERY_STAT(BotSteering),false,this);
    // Teammates are handled by separation. Scenery uses the pawn's actual blocking channel.
    for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)Query.AddIgnoredActor(*It);
    const float Radius=GetCapsuleComponent()->GetScaledCapsuleRadius()+2;
    const float Half=FMath::Max(Radius,GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-10);
    // Stop the probe at a nearby destination; looking beyond a valid tree grip
    // makes the trunk behind it appear to block the entire approach.
    const float Reach=FMath::Clamp(static_cast<float>(FVector::Dist2D(Position,Target)),12.f,bSquirrel?135.f:240.f);
    const FVector Start=Position+FVector(0,0,bSquirrel?3:10);
    const auto Shape=FCollisionShape::MakeCapsule(Radius,Half);
    FVector Best=FVector::ZeroVector;float BestScore=-MAX_flt;
    for(float Angle:{0.f,30.f,-30.f,60.f,-60.f,90.f,-90.f,125.f,-125.f,170.f}){
        const FVector Heading=Desired.RotateAngleAxis(Angle,FVector::UpVector);FVector End=Start+Heading*Reach;
        if(Forest->bLake&&AWildlineLake::RadiusAt(End)<1.1f)continue;
        if(End.Size2D()>WildlineRules::MapRadius-100)continue;
        // Follow physical support, including raised porch ramps. A flat forward
        // sweep mistakes any uphill ramp for a wall and steers away forever.
        FHitResult Floor;
        if(GetWorld()->LineTraceSingleByChannel(Floor,End+FVector(0,0,100),End-FVector(0,0,160),ECC_Pawn,Query)&&Floor.ImpactNormal.Z>.65f)
            End.Z=Floor.ImpactPoint.Z+GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+(bSquirrel?3:10);
        else End.Z+=FMath::Clamp(Forest->GroundHeight(End.X,End.Y)-Forest->GroundHeight(Start.X,Start.Y),-80.f,80.f);
        FHitResult Hit;const bool Blocked=GetWorld()->SweepSingleByChannel(Hit,Start,End,FQuat::Identity,ECC_Pawn,Shape,Query);
        const float Clearance=Blocked?Hit.Time:1.f;
        if(Clearance<.22f)continue;
        const float Side=Angle*BotSteerSide>=0?.18f:0;
        const float Score=Clearance*2+FVector::DotProduct(Heading,Desired)+Side;
        if(Score>BestScore){BestScore=Score;Best=Heading;}
        if(Angle==0&&!Blocked)return Heading;
    }
    return Best;
}

void AWildlineCharacter::MoveBot(FVector Target,float Now,bool bSeparate){
    const FVector Position=GetActorLocation();
    const FVector Destination=Target;Target=Forest->FarmRoute(Position,Target);
    const float GoalDistance=FVector::Dist2D(Position,Destination);
    if(FVector::Dist2D(BotProgressGoal,Destination)>500){BotProgressGoal=Destination;BotClosestGoalDistance=GoalDistance;BotGoalProgressAt=Now;}
    if(GoalDistance<BotClosestGoalDistance-80){BotClosestGoalDistance=GoalDistance;BotGoalProgressAt=Now;}
    const bool NoGoalProgress=Now-BotGoalProgressAt>7&&GoalDistance>150;
    if(Forest->bFarm&&!Forest->InsideFarmhouse(Position)&&Forest->InsideFarmhouse(Destination)&&(Position.X<Forest->FarmhouseLocation.X-790||FMath::Abs(Position.Y)>50)){
        // Line up before the narrow board ramp. Cutting diagonally towards the
        // doorway strands off-centre squirrels against the solid porch steps.
        const float Lane=bSquirrel?((FMath::Max(0,BotSlot)%3)-1)*24.f:0.f;
        const FVector Approach=Forest->FarmhouseLocation+FVector(-1180,Lane,0);
        if(Position.X<Approach.X-30||FMath::Abs(Position.Y-Lane)>30)Target=Approach;
        else Target=Forest->FarmhouseLocation+FVector(-720,Lane,0);
    }
    if(Now<BotEscapeUntil)Target=Position+BotEscapeHeading*800;
    if(Now-BotProgressAt>1.6f){
        const float Progress=FVector::Dist2D(Position,BotProgressPosition);
        if((Progress<45||NoGoalProgress)&&GetCharacterMovement()->IsMovingOnGround()&&FVector::Dist2D(Position,Target)>100){
            BotRecoveries++;BotSteerSide=-BotSteerSide;
            // Detect circling as well as standing still: motion must eventually
            // bring the bot closer to its objective.
            BotGoalProgressAt=Now;BotClosestGoalDistance=GoalDistance;
            // Hop low roots, lips and steps using the same jump and stamina as
            // players. Full-height walls still require the detour below.
            const FVector Heading=(Target-Position).GetSafeNormal2D();FHitResult Support,Overhead;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(BotStep),false,this);
            for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)Query.AddIgnoredActor(*It);
            const FVector Ahead=Position+Heading*110;
            if(bSquirrel&&Stamina>22&&GetWorld()->LineTraceSingleByChannel(Support,Ahead+FVector(0,0,100),Ahead-FVector(0,0,140),ECC_Pawn,Query)){
                const float Rise=Support.ImpactPoint.Z-(Position.Z-GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
                if(Rise>9&&Rise<85&&Support.ImpactNormal.Z>.65f&&!GetWorld()->LineTraceSingleByChannel(Overhead,Position+FVector(0,0,100),Ahead+FVector(0,0,100),ECC_Pawn,Query)){
                    if(Controller)Controller->SetControlRotation(Heading.Rotation());GetCharacterMovement()->StopMovementImmediately();ServerJump_Implementation();
                    BotMove=Heading;BotProgressPosition=Position;BotProgressAt=Now;return;
                }
            }
            BotEscapeHeading=(Target-Position).GetSafeNormal2D().RotateAngleAxis(BotSteerSide*105,FVector::UpVector);
            BotEscapeUntil=Now+1.4f;
            if(BotFoodIndex!=INDEX_NONE&&(!Forest->bFarm||Forest->InsideFarmhouse(Position))){BotBlockedFood.Add(BotFoodIndex,Now+18);BotFoodIndex=INDEX_NONE;BotGoalUntil=0;}
            if(!bSquirrel)BotGoalUntil=0;
            Target=Position+BotEscapeHeading*800;
        }
        BotProgressPosition=Position;BotProgressAt=Now;
    }
    BotMove=BotSteering(Target,bSeparate);
}

void AWildlineCharacter::LeaveBotTree(float Now,FVector Heading){
    ForwardInput=SideInput=0;Heading=Heading.GetSafeNormal2D();if(Heading.IsNearlyZero())Heading=GetActorForwardVector();
    // Launch away from the bark as well as the threat, avoiding a jump into the trunk.
    if(Forest->Trees.IsValidIndex(TreeIndex)){
        float Radius;const FVector Out=(GetActorLocation()-Forest->TrunkCentre(TreeIndex,GetActorLocation().Z,Radius)).GetSafeNormal2D();
        if(FVector::DotProduct(Heading,Out)<.25f)Heading=(Out+Heading*.5f).GetSafeNormal2D();
    }
    if(auto* AI=Cast<AAIController>(Controller))AI->ClearFocus(EAIFocusPriority::Gameplay);
    if(Controller)Controller->SetControlRotation(Heading.Rotation());
    if(Stamina>=15)ServerJump_Implementation();else ServerClimb_Implementation();
    BotTrackedTree=BotEscapeTree=INDEX_NONE;BotTreeCooldown=Now+7;BotGoalUntil=0;BotProgressAt=Now;
    BotProgressPosition=GetActorLocation();BotMove=Heading;
}

void AWildlineCharacter::BotThink(float Delta){
    if(!Forest||bDead||bInLobby)return;
    const float Now=ServerTime();
    if(BotDifficultyLevel!=AIDifficulty){ResetBotAwareness();BotDifficultyLevel=AIDifficulty;}
    if(TreeIndex!=INDEX_NONE){
        if(BotTrackedTree!=TreeIndex){BotTrackedTree=TreeIndex;BotTreeAt=Now;BotProgressPosition=GetActorLocation();BotProgressAt=Now;}
        const bool Course=Forest->bResidential;
        const bool TimedOut=Now-BotTreeAt>(Course?14.f:MatchRules().BotTreeSeconds);
        const bool Stuck=Now-BotProgressAt>2&&FVector::Dist(GetActorLocation(),BotProgressPosition)<20;
        if(TimedOut||Stuck||(!Course&&!MatchRules().bBotClimbing)){
            FVector Heading=BotTarget.IsValid()?GetActorLocation()-BotLastKnown:BotGoal-GetActorLocation();
            if(Course){CourseCheckpoint=0;CourseLane=-1;BotCourseStep=-1;BotPatrolStep++;Heading=GetActorLocation()-Forest->Trees[TreeIndex].Base;}
            LeaveBotTree(Now,Heading);return;
        }
        if(Now-BotProgressAt>2){BotProgressAt=Now;BotProgressPosition=GetActorLocation();}
        if(!Course){
            ForwardInput=1;SideInput=.12f*BotSteerSide;
            const auto& Tree=Forest->Trees[TreeIndex];
            if(GetActorLocation().Z>FMath::Min(Tree.Base.Z+650,Tree.ClimbTop-55))LeaveBotTree(Now,GetActorLocation()-BotLastKnown);
            return;
        }
    }else BotTrackedTree=INDEX_NONE;
    if(bSquirrel&&Forest->bResidential){CourseBotThink(Delta);return;}
    if(Now>=BotPlanAt){BotPlanAt=Now+FMath::Lerp(.25f,.04f,MatchRules().BotReactionScale);PlanBot(Now);}
    if(Controller){
        if(auto* AI=Cast<AAIController>(Controller))AI->SetFocalPoint(BotLookAt);
        Controller->SetControlRotation((BotLookAt-GetActorLocation()).Rotation());
    }
    if(TreeIndex==INDEX_NONE)AddMovementInput(BotMove,1);
}

void AWildlineCharacter::PlanBot(float Now){
    const auto& Rules=MatchRules();const FVector Position=GetActorLocation();
    TArray<AWildlineCharacter*> Peers;AWildlineCharacter* Foe=nullptr;float Nearest=MAX_flt;
    const auto Tuning=WildlineRules::HunterDifficulty(AIDifficulty);
    const float Sight=(bSquirrel?(AIDifficulty==0?1400:AIDifficulty==2?3000:2200)*Rules.CautionMultiplier():Tuning.SightRange)*Rules.SightMultiplier();
    for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It){
        if(*It==this||It->bDead||It->bInLobby||It->bPreviewHunter)continue;
        if(It->bSquirrel==bSquirrel){Peers.Add(*It);continue;}
        const float Distance=FVector::Dist2D(Position,It->GetActorLocation());
        if(Distance<Nearest&&BotCanSee(*It,Sight)){Foe=*It;Nearest=Distance;}
    }
    if(Foe){
        if(BotTarget.Get()!=Foe||Now-BotLastSeenAt>.5f)BotDecisionAt=Now+Rules.ReactionDelay(bSquirrel?(AIDifficulty==0?1.1f:AIDifficulty==2?.15f:.5f):Tuning.ReactionTime);
        BotTarget=Foe;BotLastSeenAt=Now;BotLastKnown=Foe->GetActorLocation();
    }
    if(bSquirrel){
        bAiming=false;BotLookAt=Position+GetActorForwardVector()*100;
        const bool Danger=BotTarget.IsValid()&&!BotTarget->bDead&&Now-BotLastSeenAt<2.f&&Now>=BotDecisionAt&&FVector::Dist2D(Position,BotLastKnown)<Sight;
        bSprinting=Danger&&Stamina>18;
        if(Danger&&!Forest->bFarm){
            const FVector Away=(Position-BotLastKnown).GetSafeNormal2D();
            FVector Target=Position+Away*1800;
            if(Rules.bBotClimbing&&Now>BotTreeCooldown&&Stamina>22){
                if(BotEscapeTree==INDEX_NONE||Now>BotGoalUntil){
                    float Best=MAX_flt;BotEscapeTree=INDEX_NONE;
                    for(int32 I=0;I<Forest->Trees.Num();I++){
                        const auto& Tree=Forest->Trees[I];const float D=FVector::Dist2D(Position,Tree.Base);
                        if(D>1800||FVector::Dist2D(Tree.Base,BotLastKnown)<500)continue;
                        bool Occupied=false;for(auto* Peer:Peers)if(Peer->TreeIndex==I||(Peer->bBot&&Peer->BotEscapeTree==I)){Occupied=true;break;}
                        if(Occupied)continue;
                        const float Score=D-FVector::DotProduct((Tree.Base-Position).GetSafeNormal2D(),Away)*450;
                        if(Score<Best){Best=Score;BotEscapeTree=I;}
                    }
                    BotGoalUntil=Now+3;
                }
                if(Forest->Trees.IsValidIndex(BotEscapeTree)){
                    const auto& Tree=Forest->Trees[BotEscapeTree];float Radius;const FVector Stem=Forest->TrunkCentre(BotEscapeTree,Position.Z,Radius);
                    const FVector Out=(Position-Stem).GetSafeNormal2D();Target=Stem+Out*(Radius+16);
                    if(Forest->NearestTree(Position,40)==BotEscapeTree)ServerClimb_Implementation();
                }
            }
            MoveBot(Target,Now);BotLookAt=Position+BotMove*400;return;
        }
        BotEscapeTree=INDEX_NONE;
        const auto* State=GetWorld()->GetGameState<AWildlineGameState>();
        auto Available=[&](int32 I){const float* Blocked=BotBlockedFood.Find(I);return Forest->AcornLocations.IsValidIndex(I)&&!(Forest->bFarm?State->TakenPeanuts.Contains(I):Collected.Contains(I))&&(!Blocked||*Blocked<=Now);};
        if(Bag<Rules.CarryCapacity){
            if(!Available(BotFoodIndex)||Now>BotGoalUntil){
                float Best=MAX_flt;BotFoodIndex=INDEX_NONE;
                for(int32 I=0;I<Forest->AcornLocations.Num();I++){
                    if(!Available(I))continue;const FVector Food=Forest->AcornLocations[I];float Score=FVector::Dist2D(Position,Food);
                    for(auto* Peer:Peers){
                        if(Peer->bBot&&Peer->BotFoodIndex==I)Score+=5000;
                        else if(FVector::Dist2D(Peer->GetActorLocation(),Food)<Rules.BotSpacing)Score+=600;
                    }
                    // Nearby alternatives win deterministically instead of every slot chasing the same nut.
                    Score+=((I*17+FMath::Max(0,BotSlot)*43)%97)*2;
                    if(Danger)Score+=FMath::Max(0.f,1500.f-static_cast<float>(FVector::Dist2D(Food,BotLastKnown)))*Rules.CautionMultiplier();
                    if(Score<Best){Best=Score;BotFoodIndex=I;}
                }
                BotGoalUntil=Now+12;
            }
        }else BotFoodIndex=INDEX_NONE;
        // A partial bag still goes home when no reachable food remains.
        BotGoal=BotFoodIndex!=INDEX_NONE?Forest->AcornLocations[BotFoodIndex]:Forest->DenLocation;
        MoveBot(BotGoal,Now);BotLookAt=Position+BotMove*400;return;
    }

    bAiming=Foe&&Now>=BotDecisionAt;bSprinting=false;
    if(Ammo==0)ServerReload_Implementation();
    FVector Target=BotLastKnown;bool Pursue=false;
    if(Foe){
        Target=BotLastKnown;Pursue=true;BotLookAt=Target;
        if(bAiming&&Now>=BotShotAt&&Ammo>0&&ReloadUntil<=0&&(SelectedWeapon!=1||Nearest<WildlineRules::ShotgunRange*.85f)){
            BotShotAt=Now+Rules.ShotDelay(Tuning.ShotDelay,WildlineRules::WeaponInterval(SelectedWeapon));
            const float ErrorSize=Rules.AimError(Tuning.AimError);
            const FVector Error(FMath::FRandRange(-ErrorSize,ErrorSize),FMath::FRandRange(-ErrorSize,ErrorSize),FMath::FRandRange(-ErrorSize*.45f,ErrorSize*.45f));
            const float Scale=HunterProfile.HeightScale();
            const FVector Origin=Position+FVector(0,0,72*Scale)+GetActorRightVector()*(2*Scale);
            FVector AimPoint=Target+Error;
            // Shotguns and maximum-accuracy rifles resolve immediately. Other
            // rifle shots retain ballistic travel, with motion/drop prediction.
            if(SelectedWeapon==0&&Rules.BotAccuracyScale<1.f){
                float Flight=FMath::Max(0.f,static_cast<float>(FVector::Dist(Origin,AimPoint))-65*Scale)/90000.f;
                for(int32 I=0;I<4;I++){
                    AimPoint=Target+Error+Foe->GetVelocity()*Flight+FVector(0,0,490.f*Flight*Flight);
                    Flight=FMath::Max(0.f,static_cast<float>(FVector::Dist(Origin,AimPoint))-65*Scale)/90000.f;
                }
            }
            ServerFire_Implementation((AimPoint-Origin).GetSafeNormal());
        }
        const float Stop=Rules.StopDistance(Tuning.StopDistance);
        if(Nearest<Stop){BotMove=FVector::ZeroVector;BotProgressAt=Now;BotProgressPosition=Position;return;}
    }else if(BotTarget.IsValid()&&!BotTarget->bDead&&Now-BotLastSeenAt<Rules.SearchMemory(Tuning.MemoryTime)){
        // Search the last observed place, never the hidden squirrel's current position.
        Pursue=true;const float Age=Now-BotLastSeenAt;
        if(FVector::Dist2D(Position,Target)<220)Target+=FVector(FMath::Cos(Age*1.5f+BotSlot),FMath::Sin(Age*1.5f+BotSlot),0)*500;
        BotLookAt=Target;
    }else{
        BotTarget.Reset();
        if(Now>BotGoalUntil||FVector::Dist2D(Position,BotGoal)<220){
            BotPatrolStep++;const int32 Count=Forest->AcornLocations.Num();float Best=MAX_flt;
            FVector Goal=Forest->DenLocation;
            if(Forest->bResidential){
                const int32 Lane=(BotSlot+BotPatrolStep)%2,Step=(BotPatrolStep%5);
                Goal=Forest->CourseTarget(Lane,Step);Goal+=FVector(0,Lane==0?-380:380,0);
            }else for(int32 N=0;N<FMath::Min(Count,18);N++){
                const int32 I=(FMath::Max(0,BotSlot)*13+BotPatrolStep*7+N*11)%Count;const FVector Candidate=Forest->AcornLocations[I];
                const float Distance=FVector::Dist2D(Position,Candidate);if(Distance<550)continue;
                float Score=Distance;
                for(auto* Peer:Peers)if(Peer->bBot&&FVector::Dist2D(Peer->BotGoal,Candidate)<800)Score+=2000;
                if(Score<Best){Best=Score;Goal=Candidate;}
            }
            BotGoal=Goal;BotGoalUntil=Now+16;
        }
        Target=BotGoal;BotLookAt=Position+(Target-Position).GetSafeNormal2D()*500;
        if(Ammo<WildlineRules::WeaponCapacity(SelectedWeapon)&&Now-BotLastSeenAt>3)ServerReload_Implementation();
    }
    // Offset a shared target so multiple hunters approach from different sides.
    if(Pursue&&BotSlot>=0)Target+=FVector(FMath::Cos(BotSlot*2.4f),FMath::Sin(BotSlot*2.4f),0)*140;
    MoveBot(Target,Now);
}
