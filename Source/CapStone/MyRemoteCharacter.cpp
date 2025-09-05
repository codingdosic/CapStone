// Fill out your copyright notice in the Description page of Project Settings.


#include "MyRemoteCharacter.h"

// Sets default values
AMyRemoteCharacter::AMyRemoteCharacter()
{
	PrimaryActorTick.bCanEverTick = true; // Tick 활성화 (중요!)
	bReplicates = false; // 서버 기반이 아니므로 복제는 불필요

	UE_LOG(LogTemp, Warning, TEXT("RemoteCharacter constructed."));
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

