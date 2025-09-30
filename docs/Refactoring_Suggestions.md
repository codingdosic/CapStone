# CapStone 프로젝트 코드 리팩토링 제안

현재 CapStone 프로젝트의 C++ 코드에 대한 분석 및 가독성, 유지보수성, 확장성 향상을 위한 리팩토링 제안 사항입니다.

## **총평 및 핵심 제안**

현재 코드는 기능적으로 동작하지만, 몇몇 클래스(특히 `AMyWebSocketCharacter`)가 너무 많은 역할을 담당하고 있어(God Object), 클래스 간의 결합도가 높습니다. 핵심 리팩토링 방향은 **역할과 책임의 분리(Separation of Concerns)**가 되어야 합니다. `WebSocketManager`를 더 적극적으로 활용하여 네트워크 관련 로직을 모두 위임하는 것이 좋습니다.

---

## **리팩토링 진행 상태 (2025-09-30)**

### **완료된 작업**

1.  **리팩토링 1단계: `MyRemoteCharacter` 보간 속도 변수화**
    - `MyRemoteCharacter`의 Tick 함수에 하드코딩 되어있던 보간 속도(5.0f)를 `InterpSpeed`라는 `UPROPERTY` 변수로 분리하여 블루프린트에서 수정할 수 있도록 개선했습니다.

2.  **리팩토링 2단계: 네트워크 상태 정보 소유권 이전 및 안정화**
    - `MyWebSocketCharacter`가 소유하던 `TSharedPtr<IWebSocket> WebSocket` 객체와 `FString MySocketId` 변수의 소유권을 `WebSocketManager`로 이전하는 과정에서 발생한 다수의 버그를 해결하고 코드를 안정화했습니다.
    - **컴파일 오류 해결**: `ChatWidget`이 `private` 멤버에 접근하던 문제를 `GetMySocketId()` getter 함수를 추가하여 해결했습니다.
    - **채팅 기능 정상화**: `WebSocketManager`의 웹소켓 객체가 할당되지 않아 채팅 메시지가 전송되지 않던 문제를 해결했습니다. 또한, 서버로부터 받은 ID가 `WebSocketManager`에 정상적으로 저장되지 않던 문제를 수정하여 자신의 채팅 ID가 표시되도록 했습니다.
    - **UI 포커스 오류 해결**: 채팅 시도 시 포커스를 설정할 수 없는 위젯에 접근하여 에디터가 멈추는 문제를 `ChatInputBox`에 직접 포커스를 주도록 수정하여 해결했습니다.
    - **에디터 실행 크래시 해결**: `BeginPlay` 함수 내에서 `WebSocketManager`가 생성되기 전에 호출되는 문제를 해결하여 에디터 실행 시 발생하던 크래시를 수정했습니다.
    - **웹소켓 관리 로직 일원화**: `Connect`, `Send`, `Close` 등 모든 웹소켓 관련 호출이 `WebSocketManager`를 통하도록 코드를 일원화하고, `AMyWebSocketCharacter`에 남아있던 중복 웹소켓 변수를 제거했습니다.

3.  **리팩토링 3단계: `SendTransformData` 함수 구현 이전**
    - `AMyWebSocketCharacter`의 `Tick` 함수에서 직접 호출되던 `SendTransformData` 함수의 로직을 `WebSocketManager`로 이전했습니다.
    - `AMyWebSocketCharacter`는 `Tick` 함수에서 자신의 `Transform` 정보를 `WebSocketManager`에 넘겨주기만 하고, 데이터 전송 여부 판단 및 실제 전송은 `WebSocketManager`가 담당하도록 변경했습니다.

4.  **리팩토링 4단계: `WebSocketManager` 독립성 강화 (진행 중)**
    - `WebSocketManager`의 소유권을 `AMyWebSocketCharacter`에서 `UCapStoneGameInstance`로 이전했습니다.
    - `UCapStoneGameInstance` 클래스를 생성하고 `DefaultEngine.ini`에 설정했습니다.
    - `WebSocketManager`는 `UCapStoneGameInstance`에서 생성 및 초기화되며, `AMyWebSocketCharacter`는 `GameInstance`로부터 `WebSocketManager`를 가져와 자신을 등록/해제하도록 변경했습니다.
    - `AMyWebSocketCharacter`의 `ConnectWebSocket()` 함수를 제거하고, `OtherPlayerBlueprintClass` 프로퍼티를 `UCapStoneGameInstance`로 이동했습니다.
    - **발생 문제:** 4단계 적용 후, 새로운 플레이어가 움직이거나 부딪히기 전까지 다른 클라이언트에서 보이지 않는 문제가 발생했습니다.
    - **원인 분석:** `SendTransformData`가 위치 변화가 있을 때만 전송되도록 최적화되어 있어, 초기 접속 시 위치 변화가 없으면 데이터가 전송되지 않기 때문입니다.
    - **해결 방안 (진행 중):** `UWebSocketManager`에 `bHasSentInitialTransform` 플래그를 추가하여, 연결 시 최소 한 번은 Transform 데이터를 강제로 전송하도록 수정 중입니다. (`WebSocketManager.h`에 `bHasSentInitialTransform` 추가 완료)
    - **현재 중단 지점:** `WebSocketManager.cpp` 파일에 `bHasSentInitialTransform` 플래그를 활용하는 로직을 추가하는 도중 API 오류로 인해 작업이 중단되었습니다.

### **다음 세션에서 진행할 작업 (수동 작업 필요)**

API 오류로 인해 자동 수정이 불가능하므로, 다음 세션에서는 아래 내용을 **수동으로 `WebSocketManager.cpp` 파일에 적용**해야 합니다.

**1. `WebSocketManager.cpp` 파일 열기**

**2. `Connect()` 함수 수정:**

`UWebSocketManager::Connect()` 함수를 찾아 아래와 같이 수정합니다.

**찾을 내용 (기존 `Connect()` 함수):**

```cpp
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
        WebSocket->OnMessage().AddUObject(this, &UWebSocketManager::OnWebSocketMessage);
        WebSocket->OnConnected().AddLambda([]() { UE_LOG(LogTemp, Warning, TEXT("WebSocket Connected!")); });
        WebSocket->OnConnectionError().AddLambda([](const FString& Error) { UE_LOG(LogTemp, Error, TEXT("WebSocket Connection Error: %s")), *Error); });
        WebSocket->OnClosed().AddLambda([](int32 StatusCode, const FString& Reason, bool bWasClean) { UE_LOG(LogTemp, Warning, TEXT("WebSocket Closed. Code: %d, Reason: %s, Clean: %s")), StatusCode, *Reason, (bWasClean ? TEXT("true") : TEXT("false"))); });

        WebSocket->Connect();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UWebSocketManager::Connect - Failed to create WebSocket instance."));
    }
}
```

**변경할 내용 (수정된 `Connect()` 함수):**

```cpp
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
        bHasSentInitialTransform = false; // 이 줄 추가

        WebSocket->OnMessage().AddUObject(this, &UWebSocketManager::OnWebSocketMessage);
        WebSocket->OnConnected().AddLambda([this]() // 람다 캡처 [this] 추가
        {
            UE_LOG(LogTemp, Warning, TEXT("WebSocket Connected!"));
            // 연결 성공 시 즉시 Transform 데이터 전송
            SendTransformData(); // 이 줄 추가
        });
        WebSocket->OnConnectionError().AddLambda([](const FString& Error) { UE_LOG(LogTemp, Error, TEXT("WebSocket Connection Error: %s")), *Error); });
        WebSocket->OnClosed().AddLambda([](int32 StatusCode, const FString& Reason, bool bWasClean) { UE_LOG(LogTemp, Warning, TEXT("WebSocket Closed. Code: %d, Reason: %s, Clean: %s")), StatusCode, *Reason, (bWasClean ? TEXT("true") : TEXT("false"))); });

        WebSocket->Connect();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UWebSocketManager::Connect - Failed to create WebSocket instance."));
    }
}
```

**3. `SendTransformData()` 함수 수정:**

`UWebSocketManager::SendTransformData()` 함수를 찾아 아래와 같이 수정합니다.

**찾을 내용 (기존 `SendTransformData()` 함수):**

```cpp
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
```

**변경할 내용 (수정된 `SendTransformData()` 함수):**

```cpp
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
    if (!bHasSentInitialTransform && Location.Equals(LastSentLocation, 0.01f) && Rotation.Equals(LastSentRotation, 0.01f)) // 이 줄 수정
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
    bHasSentInitialTransform = true; // 이 줄 추가

    // 위치 저장하고 다음 tick에서 비교
    LastSentLocation = Location;
    LastSentRotation = Rotation;
}
```

**4. 수정 후 빌드 및 테스트**

위 내용을 `WebSocketManager.cpp`에 수동으로 적용한 후, Visual Studio에서 프로젝트를 빌드하고 언리얼 에디터에서 게임을 실행하여 새로운 플레이어가 접속 시 즉시 보이는지 확인해 주십시오.

---

## **파일별 상세 리팩토링 제안 (최초 제안 내용)**

(기존 내용은 동일)
