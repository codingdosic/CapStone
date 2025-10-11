// Fill out your copyright notice in the Description page of Project Settings.


#include "MyRemoteCharacter.h"
#include "Components/WidgetComponent.h"
#include "NameplateWidget.h"

// Sets default values
AMyRemoteCharacter::AMyRemoteCharacter()
{
	PrimaryActorTick.bCanEverTick = true; // Tick 활성화 (중요!)
	bReplicates = true; // 원격 플레이어이므로 복제 필요
	bAlwaysRelevant = true; // 항상 클라이언트에게 관련성 있도록 설정

    NameplateComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("NameplateComponent"));
    NameplateComponent->SetupAttachment(RootComponent);
    NameplateComponent->SetWidgetSpace(EWidgetSpace::Screen);
    NameplateComponent->SetDrawSize(FVector2D(200, 30)); // 위젯 크기 조절

	UE_LOG(LogTemp, Warning, TEXT("RemoteCharacter constructed."));
}

void AMyRemoteCharacter::SetName(const FString& Name)
{
    UNameplateWidget* NameplateWidget = Cast<UNameplateWidget>(NameplateComponent->GetUserWidgetObject());
    if (NameplateWidget)
    {
        // 닉네임이 길 경우 앞 4자리만 사용
        FString DisplayName = Name.Left(4);
        NameplateWidget->SetName(DisplayName);
    }
}

// Called when the game starts or when spawned
void AMyRemoteCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyRemoteCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 부드러운 보간 이동
	FVector NewLoc = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, InterpSpeed);
	FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, InterpSpeed);
	SetActorLocationAndRotation(NewLoc, NewRot);
}

void AMyRemoteCharacter::UpdateTransformFromNetwork(const FVector& NewLoc, const FRotator& NewRot)
{
	TargetLocation = NewLoc;
	TargetRotation = NewRot;
}

