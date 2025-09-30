// Fill out your copyright notice in the Description page of Project Settings.


#include "CapStoneGameInstance.h"
#include "WebSocketManager.h"
#include "MyRemoteCharacter.h"

UCapStoneGameInstance::UCapStoneGameInstance()
{
	UE_LOG(LogTemp, Warning, TEXT("UCapStoneGameInstance::Constructor - Instance created."));
}

void UCapStoneGameInstance::Init()
{
	Super::Init();

	UE_LOG(LogTemp, Warning, TEXT("UCapStoneGameInstance::Init - GetWorld() is %s"), (GetWorld() ? TEXT("Valid") : TEXT("NULL")));
	UE_LOG(LogTemp, Warning, TEXT("UCapStoneGameInstance::Init - OtherPlayerBlueprintClass is %s"), (OtherPlayerBlueprintClass ? *OtherPlayerBlueprintClass->GetName() : TEXT("NULL")));

	// WebSocketManager 생성
	WebSocketManager = NewObject<UWebSocketManager>(this);
	// WebSocketManager 초기화 및 서버 연결 시도
	WebSocketManager->Initialize(OtherPlayerBlueprintClass, GetWorld());
	WebSocketManager->Connect();
}

void UCapStoneGameInstance::Shutdown()
{
	if (WebSocketManager)
	{
		WebSocketManager->Close();
	}

	Super::Shutdown();
}

UWebSocketManager* UCapStoneGameInstance::GetWebSocketManager() const
{
	return WebSocketManager;
}
