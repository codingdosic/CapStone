// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MyRemoteCharacter.generated.h"

UCLASS()
class CAPSTONE_API AMyRemoteCharacter : public ACharacter
{
	GENERATED_BODY()

public:
    FVector TargetLocation;
    FRotator TargetRotation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float InterpSpeed = 5.0f;

    AMyRemoteCharacter();

    void UpdateTransformFromNetwork(const FVector& NewLocation, const FRotator& NewRotation);

    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;
};
