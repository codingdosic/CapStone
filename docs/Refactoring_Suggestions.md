# CapStone 프로젝트 코드 리팩토링 제안

현재 CapStone 프로젝트의 C++ 코드에 대한 분석 및 가독성, 유지보수성, 확장성 향상을 위한 리팩토링 제안 사항입니다.

## **총평 및 핵심 제안**

현재 코드는 기능적으로 동작하지만, 몇몇 클래스(특히 `AMyWebSocketCharacter`)가 너무 많은 역할을 담당하고 있어(God Object), 클래스 간의 결합도가 높습니다. 핵심 리팩토링 방향은 **역할과 책임의 분리(Separation of Concerns)**가 되어야 합니다. `WebSocketManager`를 더 적극적으로 활용하여 네트워크 관련 로직을 모두 위임하는 것이 좋습니다.

---

## **리팩토링 진행 상태 (2025-09-05)**

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

### **남은 작업 계획**

1.  **리팩토링 3단계: `SendTransformData` 함수 구현 이전**
    - 현재 `AMyWebSocketCharacter`의 `Tick` 함수에서 직접 호출되는 `SendTransformData` 함수의 로직을 `WebSocketManager`로 이전합니다.
    - `AMyWebSocketCharacter`는 `Tick` 함수에서 자신의 `Transform` 정보를 `WebSocketManager`에 넘겨주기만 하고, 데이터 전송 여부 판단 및 실제 전송은 `WebSocketManager`가 담당하도록 변경합니다.

2.  **리팩토링 4단계: `WebSocketManager` 독립성 강화**
    - `MyWebSocketCharacter`가 `WebSocketManager`를 생성하는 현재 구조에서, `AGameStateBase` 또는 `UGameInstance`와 같은 더 상위 레벨의 클래스가 `WebSocketManager`를 생성하고 소유하도록 변경합니다.
    - 목표: 플레이어 캐릭터의 생존 여부와 관계없이 네트워크 연결이 유지되도록 합니다.

3.  **리팩토링 5단계: `MyWebSocketCharacter` 최종 정리**
    - `WebSocketManager`로 역할이 완전히 이전된 후 더 이상 필요 없어진 변수들을 삭제하여 코드를 정리합니다.
    - 대상 변수: `SendInterval`, `LastSentLocation`, `LastSentRotation`, `OtherPlayersMap` 등.

---

## **파일별 상세 리팩토링 제안 (최초 제안 내용)**

(기존 내용은 동일)
