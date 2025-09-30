// Fill out your copyright notice in the Description page of Project Settings.


#include "MyWebSocketCharacter.h"
#include "CapStoneGameInstance.h"
#include "WebSocketManager.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "ChatWidget.h"

// 생성자
AMyWebSocketCharacter::AMyWebSocketCharacter()
{
    // 매 프레임마다 Tick 수행 여부
    PrimaryActorTick.bCanEverTick = true;

}

// 캐릭터 생성 시 자동으로 호출
void AMyWebSocketCharacter::BeginPlay()
{
    // 부모의 메서드 호출
    Super::BeginPlay();

    // GameInstance에서 WebSocketManager 가져오기
    UCapStoneGameInstance* GameInstance = GetGameInstance<UCapStoneGameInstance>();
    if (GameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("AMyWebSocketCharacter::BeginPlay - GameInstance is Valid."));
        WebSocketManager = GameInstance->GetWebSocketManager();
        if (WebSocketManager)
        {
            UE_LOG(LogTemp, Warning, TEXT("AMyWebSocketCharacter::BeginPlay - WebSocketManager retrieved. Registering character."));
            // WebSocketManager에 이 캐릭터를 등록
            WebSocketManager->RegisterPlayerCharacter(this);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AMyWebSocketCharacter::BeginPlay - Failed to retrieve WebSocketManager from GameInstance."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AMyWebSocketCharacter::BeginPlay - Failed to get CapStoneGameInstance."));
    }

    if (ChatWidgetClass)
    {
        ChatWidgetInstance = CreateWidget<UChatWidget>(GetWorld(), ChatWidgetClass);

        if (ChatWidgetInstance)
        {
            ChatWidgetInstance->AddToViewport();
            ChatWidgetInstance->SetOwnerCharacter(this);
        }
    }
}

// 액터 제거나 게임 종료시 호출
void AMyWebSocketCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // WebSocketManager에서 이 캐릭터 등록 해제
    if (WebSocketManager)
    {
        WebSocketManager->UnregisterPlayerCharacter();
    }

    // 부모의 메서드 호출
    Super::EndPlay(EndPlayReason);
}

// 매 프레임마다 호출되는 함수
void AMyWebSocketCharacter::Tick(float DeltaTime)
{
    // 부모의 메서드 호출
    Super::Tick(DeltaTime);

    if (WebSocketManager)
    {
        WebSocketManager->Tick(DeltaTime);
    }
}

// 조작 설정
void AMyWebSocketCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // 부모의 메서드 호출
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAction("ToggleChat", IE_Pressed, this, &AMyWebSocketCharacter::ToggleChatInput);

}

void AMyWebSocketCharacter::ToggleChatInput()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    if (!ChatWidgetInstance && ChatWidgetClass)
    {
        ChatWidgetInstance = CreateWidget<UChatWidget>(GetWorld(), ChatWidgetClass);
        if (ChatWidgetInstance)
        {
            ChatWidgetInstance->AddToViewport();

            // 입력 모드 전환
            FInputModeGameAndUI InputMode;
            InputMode.SetWidgetToFocus(ChatWidgetInstance->ChatInputBox->TakeWidget());
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PC->SetInputMode(InputMode);
            PC->bShowMouseCursor = true;

            ChatWidgetInstance->FocusOnInput();
        }
    }
    else if (ChatWidgetInstance && ChatWidgetInstance->IsInViewport())
    {
        // 현재 입력 모드 확인해서 토글 처리
        TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetUserFocusedWidget(0);
        const bool bHasFocus = FocusedWidget == ChatWidgetInstance->ChatInputBox->TakeWidget();

        if (!bHasFocus)
        {
            // 포커스 주기
            ChatWidgetInstance->FocusOnInput();

            FInputModeGameAndUI InputMode;
            InputMode.SetWidgetToFocus(ChatWidgetInstance->ChatInputBox->TakeWidget());
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PC->SetInputMode(InputMode);
            PC->bShowMouseCursor = true;
        }
        else
        {
            // 포커스 해제 (게임 입력 모드로 복귀)
            FInputModeGameOnly GameOnly;
            PC->SetInputMode(GameOnly);
            PC->bShowMouseCursor = false;

            // 포커스 제거 (선택사항)
            FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::Cleared);
        }
    }
}

void AMyWebSocketCharacter::SendChatMessage(const FString& Message)
{
    if (WebSocketManager)
    {
        WebSocketManager->SendChatMessage(Message);
    }
}

FString AMyWebSocketCharacter::GetMySocketId() const
{
    if (WebSocketManager)
    {
        return WebSocketManager->MySocketId;
    }
    return FString();
}