// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CapStoneCharacter.h"
#include "Blueprint/UserWidget.h"
#include "MyRemoteCharacter.h"
#include "Components/EditableTextBox.h"
#include "WebSocketManager.h"
#include "MyWebSocketCharacter.generated.h"

UCLASS()
class CAPSTONE_API AMyWebSocketCharacter : public ACapStoneCharacter
{
	GENERATED_BODY()

public:
	// 생성자
	AMyWebSocketCharacter();
	// 고유 id 
	FString MySocketId;


protected:

	// 기존 부모 함수들 오버라이드
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


public:	
	// 기존 부모 함수들 오버라이드
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// 다른 플레이어 id와 인스턴스를 매핑
	UPROPERTY()
	TMap<FString, AMyRemoteCharacter*> OtherPlayersMap;

	// ChatWidget의 클래스 참조용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UChatWidget> ChatWidgetClass;

	// 생성된 인스턴스 저장용
	UPROPERTY()
	class UChatWidget* ChatWidgetInstance;

	UFUNCTION()
	void SendChatMessage(const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	FString GetMySocketId() const;


private:
    void ToggleChatInput();

    UPROPERTY()
    UWebSocketManager* WebSocketManager; // 소유하지 않고, 참조만 저장
};
