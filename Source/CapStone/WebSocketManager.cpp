#include "WebSocketManager.h"
#include "MyWebSocketCharacter.h"
#include "MyRemoteCharacter.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "ChatWidget.h"

void UWebSocketManager::Initialize(UClass* InRemoteCharacterClass, UWorld* InWorld)
{
    RemoteCharacterClass = InRemoteCharacterClass;
    World = InWorld;
    OwnerCharacter = nullptr; // 처음에는 Owner가 없음
}

void UWebSocketManager::Connect()
{
    UE_LOG(LogTemp, Warning, TEXT("UWebSocketManager::Connect - Attempting to connect..."));

    if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
    {
        FModuleManager::Get().LoadModule("WebSockets");
    }

    WebSocket = CreateWebSocket(TEXT("ws://localhost:8080"));

    if (WebSocket.IsValid())
    {
        // 새로운 연결 시 초기 전송 플래그 초기화
        bHasSentInitialTransform = false;

        WebSocket->OnMessage().AddUObject(this, &UWebSocketManager::OnWebSocketMessage);
        WebSocket->OnConnected().AddLambda([this]()
        {
            UE_LOG(LogTemp, Warning, TEXT("WebSocket Connected!"));
            // 연결 성공 시 즉시 Transform 데이터 전송
            SendTransformData();
        });
        WebSocket->OnConnectionError().AddLambda([](const FString& Error) { UE_LOG(LogTemp, Error, TEXT("WebSocket Connection Error: %s"), *Error); });
        WebSocket->OnClosed().AddLambda([](int32 StatusCode, const FString& Reason, bool bWasClean) { UE_LOG(LogTemp, Warning, TEXT("WebSocket Closed. Code: %d, Reason: %s, Clean: %s"), StatusCode, *Reason, (bWasClean ? TEXT("true") : TEXT("false"))); });

        WebSocket->Connect();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UWebSocketManager::Connect - Failed to create WebSocket instance."));
    }
}

void UWebSocketManager::Close()
{
    if (WebSocket.IsValid() && WebSocket->IsConnected())
    {
        WebSocket->Close();
    }
}

void UWebSocketManager::RegisterPlayerCharacter(AMyWebSocketCharacter* InCharacter)
{
    OwnerCharacter = InCharacter;
}

void UWebSocketManager::UnregisterPlayerCharacter()
{
    OwnerCharacter = nullptr;
}

void UWebSocketManager::OnWebSocketMessage(const FString& Message)
{
    UE_LOG(LogTemp, Log, TEXT("Received WebSocket Message: %s"), *Message);
    UE_LOG(LogTemp, Warning, TEXT("OnWebSocketMessage: OwnerCharacter is %s"), (OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("NULL")));

    // 메시지 JSON으로 파싱 준비
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

    // 파싱
    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        // 타입 확인
        FString Type = JsonObject->GetStringField("type");


        // id 일 경우 -> id 저장
        if (Type == "id")
        {
            // id 저장
            MySocketId = JsonObject->GetStringField("id");
        }


        // transform(이동)일 경우 -> 다른 플레이어 이동 반영
        else if (Type == "transform")
        {
            // 전송자 id 저장
            FString SenderId = JsonObject->GetStringField("id");

            // 내 ID면 무시
            if (SenderId == MySocketId)
            {
                return;
            }

            // 이동한 위치 정보
            FVector NewLocation(
                JsonObject->GetNumberField("x"),
                JsonObject->GetNumberField("y"),
                JsonObject->GetNumberField("z")
            );

            // 변화한 회전 정보
            FRotator NewRotation(
                JsonObject->GetNumberField("pitch"),
                JsonObject->GetNumberField("yaw"),
                JsonObject->GetNumberField("roll")
            );

            // OwnerCharacter가 유효한지 먼저 확인
            if (!OwnerCharacter)
            {
                UE_LOG(LogTemp, Error, TEXT("OnWebSocketMessage: OwnerCharacter is NULL when trying to process transform data!"));
                return;
            }

            // 만약 맵 상에 전송자 id가 없을 시 -> 새로 접속
            if (!OwnerCharacter->OtherPlayersMap.Contains(SenderId))
            {
                // 블루프린트 없을 시 오류 출력
                if (!RemoteCharacterClass)
                {
                    UE_LOG(LogTemp, Error, TEXT("OtherPlayerBlueprintClass is NULL!"));
                    return;
                }

                // 액터 스폰 설정 구조체
                FActorSpawnParameters SpawnParams;
                // 생성 시 충돌해도 생성 수행
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

                // 현재 월드에 AMyRemoteCharacter의 블루프린트로 T형 액터를 동적으로 전달받은 위치에 생성
                AMyRemoteCharacter* NewPlayer = World->SpawnActor<AMyRemoteCharacter>(
                    RemoteCharacterClass, NewLocation + FVector(0.0f, (OwnerCharacter->OtherPlayersMap.Num() + 1) * 100.0f, 0.0f), NewRotation, SpawnParams);

                // 생성 성공 시
                if (NewPlayer)
                {
                    // 다른 플레이어들 정보에 저장
                    OwnerCharacter->OtherPlayersMap.Add(SenderId, NewPlayer);
                    // 생성 성공 로그 출력
                    UE_LOG(LogTemp, Warning, TEXT("Spawning new remote player for ID: %s at %s"), *SenderId, *NewPlayer->GetActorLocation().ToString());
                }
                // 생성 실패 시 
                else
                {
                    // 생성 실패 로그 출력
                    UE_LOG(LogTemp, Error, TEXT("Failed to spawn remote player for ID: %s"), *SenderId);
                }
            }
            // 만일 이미 스폰되어 있는 캐릭터라면
            else
            {
                // 다른 플레이어 정보 불러오기
                AMyRemoteCharacter* OtherPlayer = OwnerCharacter->OtherPlayersMap[SenderId];

                // 정보가 있다면
                if (OtherPlayer)
                {
                    // 위치/회전을 갱신
                    OtherPlayer->UpdateTransformFromNetwork(NewLocation, NewRotation);
                }
            }
        }
        else if (Type == "chat")
        {
            FString SenderId = JsonObject->GetStringField("id");
            FString ChatText = JsonObject->GetStringField("message");

            if (SenderId == MySocketId)
            {
                // 자기 메시지는 무시
                return;
            }

            if (OwnerCharacter && OwnerCharacter->ChatWidgetInstance)
            {
                // UI에 메시지 표시
                OwnerCharacter->ChatWidgetInstance->AddChatMessage(SenderId, ChatText);
            }
        }


    }
}

TSharedPtr<IWebSocket> UWebSocketManager::CreateWebSocket(const FString& Url)
{
    if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
    {
        FModuleManager::Get().LoadModule("WebSockets");
    }

    return FWebSocketsModule::Get().CreateWebSocket(Url);
}

void UWebSocketManager::SendChatMessage(const FString& ChatMessage)
{
    if (WebSocket.IsValid() && WebSocket->IsConnected())
    {
        // JSON 형태로 메시지 구성
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
        JsonObject->SetStringField("type", "chat");
        JsonObject->SetStringField("id", MySocketId);
        JsonObject->SetStringField("message", ChatMessage);

        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

        // 웹소켓으로 메시지 전송
        WebSocket->Send(OutputString);
    }
}

void UWebSocketManager::Tick(float DeltaTime)
{
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

void UWebSocketManager::SendTransformData()
{
    // 웹소켓 또는 오너 캐릭터가 유효하지 않으면 종료
    if (!WebSocket.IsValid() || !WebSocket->IsConnected() || !OwnerCharacter)
    {
        return;
    }

    // 위치 저장
    FVector Location = OwnerCharacter->GetActorLocation();
    // 회전 저장
    FRotator Rotation = OwnerCharacter->GetActorRotation();

    // 초기 전송이 아니면서, 미세한 이동도 없으면 전송하지 않음
    if (!bHasSentInitialTransform && Location.Equals(LastSentLocation, 0.01f) && Rotation.Equals(LastSentRotation, 0.01f))
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

    // 초기 전송 플래그 설정
    bHasSentInitialTransform = true;

    // 위치 저장하고 다음 tick에서 비교
    LastSentLocation = Location;
    LastSentRotation = Rotation;
}

