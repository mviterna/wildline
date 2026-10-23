#pragma once
#include "CoreMinimal.h"

// A compressed, playable interpretation of Knobs State Forest. X is east,
// positive Y is south; these are game coordinates, not survey coordinates.
namespace WildlineKnobs {
inline float CreekY(float X){return -3900.f+650.f*FMath::Sin(X*.00048f)+270.f*FMath::Sin(X*.00105f);}
inline float RoadY(float X){return CreekY(X)-1150.f+2300.f*FMath::SmoothStep(200.f,2800.f,X);}
inline const TArray<FVector2D>& Trail(int32 Route){
    static const TArray<TArray<FVector2D>> Routes=[](){
        const TArray<TArray<FVector2D>> Corners={
            {{-8200,-6000},{-6800,-2800},{-8200,800},{-4200,3800},{-1800,1300},{-2900,-2400},{-5200,-4100}},
            {{-2500,-3000},{0,-900},{3500,1500},{6700,-1200},{4800,-3500},{600,-4700}},
            {{-2000,1000},{-800,6000},{4000,8200},{7900,7400},{9200,3500},{5300,1100},{1500,3100}}};
        TArray<TArray<FVector2D>> Result;
        for(const auto& C:Corners){TArray<FVector2D> P;for(int32 I=0;I<C.Num();I++)for(int32 J=0;J<8;J++){
            const float T=J/8.f;const auto A=C[(I+C.Num()-1)%C.Num()],B=C[I],D=C[(I+1)%C.Num()],E=C[(I+2)%C.Num()];
            P.Add(.5f*((2*B)+(-A+D)*T+(2*A-5*B+4*D-E)*T*T+(-A+3*B-3*D+E)*T*T*T));
        }const FVector2D First=P[0];P.Add(First);Result.Add(P);}return Result;
    }();return Routes[FMath::Clamp(Route,0,2)];
}
inline float TrailDistance(float X,float Y){
    const FVector2D P(X,Y);float Best=MAX_flt;
    for(int32 R=0;R<3;R++){const auto& Points=Trail(R);for(int32 I=1;I<Points.Num();I++){
        const auto A=Points[I-1],D=Points[I]-A;const float T=FMath::Clamp(FVector2D::DotProduct(P-A,D)/D.SizeSquared(),0.,1.);
        Best=FMath::Min(Best,static_cast<float>(FVector2D::Distance(P,A+D*T)));
    }}return Best;
}
inline FVector2D Parking(int32 I){return I==0?FVector2D(-8200,-6000):I==1?FVector2D(3600,RoadY(3600)-480):FVector2D(7900,7400);}
inline bool InParking(float X,float Y,float Margin=0){for(int32 I=0;I<3;I++){const auto P=Parking(I);if(FMath::Abs(X-P.X)<620+Margin&&FMath::Abs(Y-P.Y)<390+Margin)return true;}return false;}
inline bool Meadow(float X,float Y){return FMath::Square((X+4400)/2000)+FMath::Square((Y+6000)/1150)<1||FMath::Square((X-2800)/1700)+FMath::Square((Y+6000)/900)<1;}
inline bool OpenGround(float X,float Y,float Margin=0){return FMath::Abs(Y-CreekY(X))<210+Margin||FMath::Abs(Y-RoadY(X))<170+Margin||InParking(X,Y,Margin)||TrailDistance(X,Y)<100+Margin;}
inline float Ground(float X,float Y){
    auto Hill=[&](float CX,float CY,float Width,float Height){return Height*FMath::Exp(-(FMath::Square(X-CX)+FMath::Square(Y-CY))/FMath::Square(Width));};
    const float Ridges=Hill(-6200,2600,3700,1550)+Hill(4600,3200,4200,2200)+Hill(7000,-7000,3300,1150)+Hill(-10500,-8500,3400,950);
    const float D=FMath::Abs(Y-CreekY(X));
    const float Valley=FMath::SmoothStep(200.f,2400.f,D);
    const float Bed=80+24*FMath::Sin(X*.0006f);
    const float Ripple=(55*FMath::Sin(X*.0008f)*FMath::Cos(Y*.0007f)+24*FMath::Sin((X+Y)*.0015f))*Valley;
    const float Bank=75*FMath::SmoothStep(115.f,380.f,D);
    const float Rim=FMath::SmoothStep(14300.f,21500.f,static_cast<float>(FVector2D(X,Y).Size()));
    return Bed+Bank+Ridges*Valley+Ripple+Rim*Rim*2500;
}
}
