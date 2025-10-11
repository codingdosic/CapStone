// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IWebSocket.h"
#include "UObject/NoExportTypes.h"
#include "WebSocketManager.generated.h"

class AMyRemoteCharacter;
class AMyWebSocketCharacter;
class UChatWidget;

UCLASS()
class CAPSTONE_API UWebSocketManager : public UObject
{
	GENERATED_BODY()
	
public:

	// 웹소켓 객체
	TSharedPtr<IWebSocket> WebSocket;

	// 소켓 고유 ID
	FString MySocketId;

    // 클래스, 월드 초기 참조값 설정
    void Initialize(UClass* InRemoteCharacterClass, UWorld* InWorld);

	// 웹소켓 연결
	void Connect();

	// 웹소켓 연결 종료
	void Close();

	// 플레이어 캐릭터 등록
	void RegisterPlayerCharacter(AMyWebSocketCharacter* InCharacter);

	// 플레이어 캐릭터 등록 해제
	void UnregisterPlayerCharacter();

    void OnWebSocketMessage(const FString& Message);

    TSharedPtr<IWebSocket> CreateWebSocket(const FString& Url);

    void SendChatMessage(const FString& ChatMessage);

    // 캐릭터의 Tick에서 호출될 함수
    void Tick(float DeltaTime);

private:

    // 캐릭터 상태정보 변환 및 전송
    void SendTransformData();

    // 전송 간격
    float TimeSinceLastSend = 0.0f;
    float SendInterval = 0.1f;

    // 마지막으로 전송한 위치/회전/속도/점프 상태
    FVector LastSentLocation;
    FRotator LastSentRotation;
    float LastSentSpeed;
    bool LastSentIsFalling;

    // 원격 캐릭터 클래스
    UClass* RemoteCharacterClass;

    // 월드 참조
    UWorld* World;

    // 소유 캐릭터 (위치/회전 참조용)
    UPROPERTY()
    AMyWebSocketCharacter* OwnerCharacter;

    // 다른 플레이어들을 ID로 매핑
    TMap<FString, AMyRemoteCharacter*> OtherPlayersMap;

    // 초기 Transform 데이터를 전송했는지 여부
    bool bHasSentInitialTransform = false;
};
