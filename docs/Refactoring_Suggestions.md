# CapStone 프로젝트 코드 리팩토링 제안

현재 CapStone 프로젝트의 C++ 코드에 대한 분석 및 가독성, 유지보수성, 확장성 향상을 위한 리팩토링 제안 사항입니다.

## **총평 및 핵심 제안**

현재 코드는 기능적으로 동작하지만, 몇몇 클래스(특히 `AMyWebSocketCharacter`)가 너무 많은 역할을 담당하고 있어(God Object), 클래스 간의 결합도가 높습니다. 핵심 리팩토링 방향은 **역할과 책임의 분리(Separation of Concerns)**가 되어야 합니다. `WebSocketManager`를 더 적극적으로 활용하여 네트워크 관련 로직을 모두 위임하는 것이 좋습니다.

---

## **파일별 상세 리팩토링 제안**

### **1. `MyWebSocketCharacter.h` / `MyWebSocketCharacter.cpp`**

- **문제점:**
    1.  **과도한 책임**: 플레이어 조작, WebSocket 연결 생명주기 관리, 데이터 전송, 다른 플레이어 목록(`OtherPlayersMap`) 관리, UI 생성 등 너무 많은 역할을 수행하고 있습니다.
    2.  **높은 결합도**: `WebSocketManager`, `ChatWidget`, `MyRemoteCharacter` 등 여러 클래스와 직접적으로 강하게 연결되어 있습니다.
    3.  **하드코딩된 값**: 데이터 전송 주기(`SendInterval = 0.1f`)가 코드에 고정되어 있어 유연성이 떨어집니다.

- **개선 방안:**
    1.  **네트워크 로직 완전 위임**:
        - `TSharedPtr<IWebSocket> WebSocket` 객체를 `WebSocketManager`로 이동시킵니다.
        - `ConnectWebSocket`, `SendTransformData`, `SendChatMessage` 함수의 실제 구현을 `WebSocketManager`로 옮기고, 캐릭터는 `WebSocketManager`의 함수를 호출만 하도록 변경합니다.
        - `OtherPlayersMap`을 `WebSocketManager`로 이동시켜 원격 플레이어 관리를 완전히 위임합니다.
    2.  **캐릭터의 역할 축소**:
        - 캐릭터는 순수하게 플레이어의 입력을 받아 `WebSocketManager`에 "데이터를 보내달라"고 요청하는 역할만 수행해야 합니다.
        - 예: `WebSocketManager->SendTransform(GetActorTransform());`
    3.  **하드코딩된 값 변수화**:
        - `SendInterval`을 `UPROPERTY(EditAnywhere)`로 선언하여 블루프린트에서 쉽게 수정할 수 있도록 변경합니다.

### **2. `WebSocketManager.h` / `WebSocketManager.cpp`**

- **문제점:**
    - 'Manager'라는 이름에 비해 역할이 제한적입니다. 현재는 메시지 파싱 및 원격 캐릭터 생성/업데이트만 담당하고 있어 수동적인 역할에 가깝습니다.

- **개선 방안:**
    1.  **네트워크 제어권 완전 이전**:
        - `MyWebSocketCharacter`로부터 WebSocket 연결, 해제, 데이터 송수신 등 모든 네트워크 관련 제어권을 가져옵니다.
        - `IWebSocket` 객체를 직접 소유하고 관리합니다.
    2.  **상태 관리 강화**:
        - `OtherPlayersMap`을 소유하고, 원격 플레이어의 생성, 업데이트, 소멸을 모두 책임집니다.
    3.  **독립성 확보**:
        - `GameMode`나 `GameState`, `GameInstance` 등 게임의 공용 클래스에서 `WebSocketManager`를 생성하고 소유하도록 변경합니다. 이렇게 하면 플레이어 캐릭터의 생명주기와 상관없이 네트워크 연결을 유지할 수 있습니다.
    4.  **명확한 API 제공**:
        - `Connect(const FString& Url)`, `Disconnect()`, `SendChatMessage(const FString& Message)`, `SendTransform(const FTransform& Data)` 등 명확한 공개 함수(API)를 제공하여 다른 클래스들이 쉽게 사용할 수 있도록 합니다.

### **3. `ChatWidget.h` / `ChatWidget.cpp`**

- **문제점:**
    - `AMyWebSocketCharacter` 클래스와 직접적으로 강하게 결합되어 있어, 다른 종류의 캐릭터가 이 위젯을 재사용하기 어렵습니다.

- **개선 방안:**
    1.  **결합도 낮추기 (중급 리팩토링)**:
        - `OwnerCharacter` 참조 대신, `WebSocketManager`에 대한 참조를 `GameState` 등에서 가져와 직접 통신하는 것이 더 좋습니다. (`WebSocketManager->SendChatMessage(...)`)
    2.  **결합도 낮추기 (고급 리팩토링)**:
        - C#의 `event`처럼, 델리게이트(Delegate)를 선언하여 메시지가 입력되었음을 외부에 알리기만 하고, 실제 전송은 이 델리게이트에 바인딩된 다른 객체(예: `WebSocketManager`)가 처리하도록 변경할 수 있습니다.

### **4. `MyRemoteCharacter.h` / `MyRemoteCharacter.cpp`**

- **문제점:**
    - 기능적으로 큰 문제는 없으나, 다른 플레이어의 움직임을 보간(Interpolation)하는 속도(`5.0f`)가 하드코딩되어 있습니다.

- **개선 방안:**
    - `SetActorLocationAndRotation`에 사용되는 보간 속도를 `UPROPERTY(EditAnywhere)` 변수로 만들어 블루프린트에서 조절할 수 있도록 하면 테스트 및 수정이 용이해집니다.

### **5. `CapStoneGameMode.h` / `CapStoneGameMode.cpp`**

- **문제점:**
    - 현재 거의 아무 역할도 하고 있지 않습니다.

- **개선 방안:**
    - `WebSocketManager`를 생성하고 소유하기에 적합한 후보 중 하나입니다. 게임의 전반적인 흐름을 관리하는 `GameMode`에서 `WebSocketManager`를 초기화하면 코드 구조가 더 명확해질 수 있습니다. (단, `GameMode`는 서버에만 존재하므로, 현재 구조에서는 `GameState`나 `GameInstance`가 더 적합할 수 있습니다.)
