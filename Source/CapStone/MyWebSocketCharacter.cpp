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

    // 액터 위치 저장
    LastSentLocation = GetActorLocation();
    // 액터 회전 저장
    LastSentRotation = GetActorRotation();

    WebSocketManager = NewObject<UWebSocketManager>(this);
    WebSocketManager->Initialize(OtherPlayerBlueprintClass, GetWorld(), this);

}

// 액터 제거나 게임 종료시 호출
void AMyWebSocketCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 서버에 연결되어 있으면 실행
    if (WebSocket.IsValid() && WebSocket->IsConnected())
    {
        // 연결 종료
        WebSocket->Close();
    }

    // 부모의 메서드 호출
    Super::EndPlay(EndPlayReason);
}

// 매 프레임마다 호출되는 함수
void AMyWebSocketCharacter::Tick(float DeltaTime)
{
    // 부모의 메서드 호출
    Super::Tick(DeltaTime);

    // 마지막 전송 이후 경과 시간
    TimeSinceLastSend += DeltaTime;

    // 경과 시간이 전송 간격을 넘어가면 실행  
    if (TimeSinceLastSend >= SendInterval)
    {
        // 데이터 전송
        SendTransformData();

        // 경과 시간 초기화
        TimeSinceLastSend = 0.0f;
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
    WebSocket = WebSocketManager->CreateWebSocket(TEXT("ws://0.tcp.jp.ngrok.io:14176"));

    // 메시지 수신 콜백
    WebSocket->OnMessage().AddLambda([this](const FString& Message)
        {
            // 메시지 처리
            //OnWebSocketMessage(Message);
            WebSocketManager->OnWebSocketMessage(Message);
        });


    // 웹소켓 연결 시도
    WebSocket->Connect();
}


// 매 프레임마다 데이터 전송하는 함수
void AMyWebSocketCharacter::SendTransformData()
{
    // 웹소켓 유효하지 않으면 종료
    if (!WebSocket.IsValid() || !WebSocket->IsConnected()) return;

    // 위치 저장
    FVector Location = GetActorLocation();
    // 회전 저장
    FRotator Rotation = GetActorRotation();

    // 미세한 이동은 무시하기
    if (Location.Equals(LastSentLocation, 0.01f) && Rotation.Equals(LastSentRotation, 0.01f))
    {
        return;
    }

    // JSON 객체 생성
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField("id", MySocketId); // id
    JsonObject->SetStringField("type", "transform"); // 이동
    JsonObject->SetNumberField("x", Location.X); // 위치
    JsonObject->SetNumberField("y", Location.Y);
    JsonObject->SetNumberField("z", Location.Z);
    JsonObject->SetNumberField("pitch", Rotation.Pitch); // 회전
    JsonObject->SetNumberField("yaw", Rotation.Yaw);
    JsonObject->SetNumberField("roll", Rotation.Roll);

    // JSON -> FString 으로 직렬화 
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    // 직렬화한 데이터 서버로 전송
    WebSocket->Send(OutputString);

    // 위치 저장하고 다음 tick에서 비교
    LastSentLocation = Location;
    LastSentRotation = Rotation;


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
            InputMode.SetWidgetToFocus(ChatWidgetInstance->TakeWidget());
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
            InputMode.SetWidgetToFocus(ChatWidgetInstance->TakeWidget());
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
    if (!WebSocket.IsValid() || !WebSocket->IsConnected())
        return;

    // JSON 메시지 생성
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField("id", MySocketId);
    JsonObject->SetStringField("type", "chat"); // 타입: 채팅 메시지
    JsonObject->SetStringField("message", Message);

    // JSON -> 문자열 직렬화
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    // 웹소켓으로 전송
    WebSocket->Send(OutputString);
}


