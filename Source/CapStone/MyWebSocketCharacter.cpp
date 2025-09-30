// Fill out your copyright notice in the Description page of Project Settings.


#include "MyWebSocketCharacter.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "MyRemoteCharacter.h" // 다른 플레이어용 클래스
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
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

    // WebSocketManager를 먼저 생성하고 초기화합니다.
    WebSocketManager = NewObject<UWebSocketManager>(this);
    WebSocketManager->Initialize(OtherPlayerBlueprintClass, GetWorld(), this);

    if (ChatWidgetClass)
    {
        ChatWidgetInstance = CreateWidget<UChatWidget>(GetWorld(), ChatWidgetClass);

        if (ChatWidgetInstance)
        {
            ChatWidgetInstance->AddToViewport();
            ChatWidgetInstance->SetOwnerCharacter(this);
        }
    }

    // 모듈을 동적으로 로드
    FModuleManager::Get().LoadModuleChecked<FWebSocketsModule>("WebSockets");
    ConnectWebSocket();
}

// 액터 제거나 게임 종료시 호출
void AMyWebSocketCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 서버에 연결되어 있으면 실행
    if (WebSocketManager && WebSocketManager->WebSocket.IsValid() && WebSocketManager->WebSocket->IsConnected())
    {
        // 연결 종료
        WebSocketManager->WebSocket->Close();
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

// 웹소켓 객체 생성
void AMyWebSocketCharacter::ConnectWebSocket()
{
    // 웹소켓 모듈에서 인스턴스를 생성하고, 로컬 서버에 연결함
    WebSocketManager->WebSocket = WebSocketManager->CreateWebSocket(TEXT("ws://localhost:8080"));


    // 메시지 수신 콜백
    WebSocketManager->WebSocket->OnMessage().AddLambda([this](const FString& Message)
        {
            // 메시지 처리
            //OnWebSocketMessage(Message);
            WebSocketManager->OnWebSocketMessage(Message);
        });


    // 웹소켓 연결 시도
    WebSocketManager->WebSocket->Connect();
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
    if (!WebSocketManager || !WebSocketManager->WebSocket.IsValid() || !WebSocketManager->WebSocket->IsConnected())
        return;

    // JSON 메시지 생성
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField("id", WebSocketManager->MySocketId);
    JsonObject->SetStringField("type", "chat"); // 타입: 채팅 메시지
    JsonObject->SetStringField("message", Message);

    // JSON -> 문자열 직렬화
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    // 웹소켓으로 전송
    WebSocketManager->WebSocket->Send(OutputString);
}

FString AMyWebSocketCharacter::GetMySocketId() const
{
    if (WebSocketManager)
    {
        return WebSocketManager->MySocketId;
    }
    return FString();
}