// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MyRemoteCharacter.generated.h"

class UWidgetComponent;

UCLASS()
class CAPSTONE_API AMyRemoteCharacter : public ACharacter
{
	GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UWidgetComponent* NameplateComponent;

    void SetName(const FString& Name);

    // UPROPERTY()를 추가하여 엔진이 안전하게 인식하도록 수정
    UPROPERTY(VisibleAnywhere, Category = "Movement")
    FVector TargetLocation;

    // UPROPERTY()를 추가하여 엔진이 안전하게 인식하도록 수정
    UPROPERTY(VisibleAnywhere, Category = "Movement")
    FRotator TargetRotation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float InterpSpeed = 5.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    float CurrentSpeed;

    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    bool bIsFalling;

    AMyRemoteCharacter();

    void UpdateTransformFromNetwork(const FVector& NewLocation, const FRotator& NewRotation, float NewSpeed, bool bNewIsFalling);

    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;
};
