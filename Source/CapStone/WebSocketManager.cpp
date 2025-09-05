#include "WebSocketManager.h"
#include "MyWebSocketCharacter.h"
#include "MyRemoteCharacter.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "ChatWidget.h"

void UWebSocketManager::Initialize(UClass* InRemoteCharacterClass, UWorld* InWorld, AMyWebSocketCharacter* InOwnerCharacter)
{
    RemoteCharacterClass = InRemoteCharacterClass;
    World = InWorld;
    OwnerCharacter = InOwnerCharacter;
}

void UWebSocketManager::OnWebSocketMessage(const FString& Message)
{
    UE_LOG(LogTemp, Log, TEXT("Received WebSocket Message: %s"), *Message);

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
            OwnerCharacter->MySocketId = JsonObject->GetStringField("id");
        }


        // transform(이동)일 경우 -> 다른 플레이어 이동 반영
        else if (Type == "transform")
        {
            // 전송자 id 저장
            FString SenderId = JsonObject->GetStringField("id");

            // 내 ID면 무시
            if (SenderId == OwnerCharacter->MySocketId)
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
                    RemoteCharacterClass, NewLocation, NewRotation, SpawnParams);

                // 생성 성공 시
                if (NewPlayer)
                {
                    // 다른 플레이어들 정보에 저장
                    OwnerCharacter->OtherPlayersMap.Add(SenderId, NewPlayer);
                    // 생성 성공 로그 출력
                    UE_LOG(LogTemp, Warning, TEXT("Spawning new remote player for ID: %s"), *SenderId);
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

            if (SenderId == OwnerCharacter->MySocketId)
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
        JsonObject->SetStringField("id", OwnerCharacter->MySocketId);
        JsonObject->SetStringField("message", ChatMessage);

        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

        // 웹소켓으로 메시지 전송
        WebSocket->Send(OutputString);
    }
}

