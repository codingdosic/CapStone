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

    // 클래스, 월드, 로컬 캐릭터 등 초기 참조값 설정
    void Initialize(UClass* InRemoteCharacterClass, UWorld* InWorld, AMyWebSocketCharacter* InOwnerCharacter);

    void OnWebSocketMessage(const FString& Message);

    TSharedPtr<IWebSocket> CreateWebSocket(const FString& Url);

    void SendChatMessage(const FString& ChatMessage);

private:

private:
    // 웹소켓 객체
    TSharedPtr<IWebSocket> WebSocket;

    // 소켓 고유 ID
    FString MySocketId;

    // 마지막으로 전송한 위치/회전
    FVector LastSentLocation;
    FRotator LastSentRotation;

    // 원격 캐릭터 클래스
    UClass* RemoteCharacterClass;

    // 월드 참조
    UWorld* World;

    // 소유 캐릭터 (위치/회전 참조용)
    UPROPERTY()
    AMyWebSocketCharacter* OwnerCharacter;

    // 다른 플레이어들을 ID로 매핑
    TMap<FString, AMyRemoteCharacter*> OtherPlayersMap;
};
