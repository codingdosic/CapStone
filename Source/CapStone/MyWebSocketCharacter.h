// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CapStoneCharacter.h"
#include "IWebSocket.h"
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


private:


	// 웹소켓 객체
	TSharedPtr<IWebSocket> WebSocket;

	// 서버 연결 및 콜백 바인딩
	void ConnectWebSocket();

	

	// 캐릭터 상태정보 변환 및 전송
	void SendTransformData();

	void ToggleChatInput();

	// 전송 간격
	float TimeSinceLastSend = 0.0f;
	float SendInterval = 0.1f;

	// 최근 위치/회전
	FVector LastSentLocation;
	FRotator LastSentRotation;

	// 블루프린트에서 설정할 수 있도록 캐릭터 클래스도 선언
	UPROPERTY(EditAnywhere, Category = "WebSocket")
	TSubclassOf<AMyRemoteCharacter> OtherPlayerBlueprintClass;

	UPROPERTY()
	UWebSocketManager* WebSocketManager;
};
