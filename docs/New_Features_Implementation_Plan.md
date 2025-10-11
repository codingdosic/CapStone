# 신규 기능 구현 계획서 (닉네임 표시, 애니메이션 동기화)

이 문서는 플레이어 머리 위에 닉네임을 표시하고, 다른 플레이어의 애니메이션을 동기화하는 두 가지 핵심 기능의 구현 계획을 상세히 기술합니다.

**주의:** 이 계획은 현재 프로젝트에 존재하는 "신규 접속 플레이어가 움직이기 전까지 보이지 않는 버그"가 해결되지 않은 상태에서 수립되었습니다. 해당 버그로 인해 구현 과정이나 결과에 예기치 못한 문제가 발생할 수 있습니다.

---

## 1. 기능 1: 플레이어 닉네임 표시

**목표:** 모든 플레이어(로컬 및 원격)의 머리 위에 각자의 닉네임(채팅 ID의 일부)을 표시하는 위젯을 추가합니다.

### 1.1. 구현 단계

#### **1단계: 이름표 위젯 C++ 클래스 생성**

-   `NameplateWidget.h` 및 `NameplateWidget.cpp` 파일을 새로 생성하여 `UUserWidget`을 상속받는 `UNameplateWidget` 클래스를 구현합니다.
-   이 클래스 내에 `UPROPERTY`로 `UTextBlock*` 변수를 선언하여 닉네임을 표시할 텍스트 컴포넌트를 바인딩할 수 있도록 합니다.
-   닉네임을 설정하는 `SetName(const FString& Name)` 함수를 공개(public)로 선언합니다.

#### **2단계: UMG 모듈 추가**

-   `Source/CapStone/CapStone.Build.cs` 파일을 열어 `PublicDependencyModuleNames` 배열에 `"UMG"` 모듈을 추가합니다. 이는 언리얼 모션 그래픽(UI) 기능을 사용하기 위해 필수적입니다.

#### **3단계: 캐릭터에 위젯 컴포넌트 추가**

-   `AMyWebSocketCharacter.h`와 `AMyRemoteCharacter.h` 두 클래스 모두에 `UWidgetComponent*` 타입의 `NameplateComponent`를 `UPROPERTY`로 추가합니다.
-   각 캐릭터의 생성자(`.cpp`)에서 `CreateDefaultSubobject`를 사용하여 `NameplateComponent`를 생성하고, 캐릭터의 `CapsuleComponent`에 부착(Attach)합니다.
-   위젯 컴포넌트의 공간(Space) 설정을 `Screen`으로 하여 이름표가 항상 화면을 바라보게 만듭니다.

#### **4.단계: 캐릭터 블루프린트 및 위젯 블루프린트 설정 (에디터 작업)**

-   언리얼 에디터에서 `UNameplateWidget` C++ 클래스를 기반으로 하는 새로운 위젯 블루프린트(예: `WBP_Nameplate`)를 생성합니다.
-   `WBP_Nameplate`를 열고, C++에 선언된 `UTextBlock*` 변수와 위젯의 텍스트 블록을 연결합니다.
-   `BP_ThirdPersonCharacter` (또는 관련 캐릭터 블루프린트)를 열고, C++에서 추가한 `NameplateComponent`의 `Widget Class`를 방금 생성한 `WBP_Nameplate`로 설정합니다.

#### **5단계: 닉네임 데이터 설정**

-   **로컬 플레이어:** `AMyWebSocketCharacter`가 `UCapStoneGameInstance`를 통해 `WebSocketManager`로부터 자신의 `MySocketId`를 받은 후, `NameplateComponent`의 위젯을 가져와 `SetName()` 함수를 호출하여 자신의 닉네임을 설정합니다.
-   **원격 플레이어:** `UWebSocketManager::OnWebSocketMessage()` 함수를 수정합니다. 서버로부터 받은 플레이어 데이터에서 `id` 값을 파싱합니다.
-   해당 `id`를 가진 `AMyRemoteCharacter`를 찾거나 새로 스폰한 후, 해당 캐릭터의 `SetName()` 함수를 호출하여 닉네임을 위젯에 표시합니다.

---

## 2. 기능 2: 원격 플레이어 애니메이션 동기화

**목표:** 플레이어의 이동 상태(속도)를 서버와 동기화하여, 다른 플레이어들에게 걷거나 멈추는 애니메이션이 정상적으로 보이게 합니다.

### 2.1. 구현 단계

#### **1단계: 데이터 전송 구조 변경 (클라이언트 -> 서버)**

-   `UWebSocketManager::SendTransformData()` 함수의 시그니처를 변경하여 캐릭터의 속도(Velocity)를 인자로 받도록 수정합니다. (예: `SendTransformData(const FVector& Velocity)`)
-   `AMyWebSocketCharacter::Tick()` 함수에서 `GetVelocity().Size()`를 통해 계산된 현재 속도(speed)를 `WebSocketManager`의 `SendTransformData()` 함수로 전달합니다.
-   `UWebSocketManager`는 `SendTransformData` 함수 내에서, 전송할 JSON 데이터에 `"speed"` 필드를 추가하여 현재 속도 값을 담아 서버로 보냅니다.

#### **2단계: 서버 로직 수정 (가정)**

-   Node.js 서버는 클라이언트로부터 받은 JSON 데이터에서 `"speed"` 필드를 파싱해야 합니다.
-   서버는 특정 플레이어의 `transform`과 `speed` 데이터를 수신하면, 해당 플레이어를 제외한 모든 다른 클라이언트에게 이 정보를 브로드캐스팅해야 합니다.

#### **3단계: 데이터 수신 처리 (서버 -> 클라이언트)**

-   `UWebSocketManager::OnWebSocketMessage()` 함수를 수정하여, 서버로부터 받은 JSON 데이터에 `"speed"` 필드가 있는지 확인하고 값을 파싱합니다.

#### **4.단계: 원격 캐릭터 애니메이션 상태 업데이트**

-   `AMyRemoteCharacter.h` 파일에 `UPROPERTY(BlueprintReadOnly)` 지정자가 붙은 `float RemoteSpeed;` 변수를 추가합니다. 이 변수는 애니메이션 블루프린트에서 읽기 전용으로 사용됩니다.
-   `UWebSocketManager::OnWebSocketMessage()`에서 파싱한 `speed` 값을 사용하여, 해당 원격 플레이어를 식별하고 그 캐릭터의 `RemoteSpeed` 변수 값을 업데이트합니다.

#### **5단계: 애니메이션 블루프린트 수정 (에디터 작업)**

-   언리얼 에디터에서 `AMyRemoteCharacter`가 사용하는 애니메이션 블루프린트를 엽니다.
-   이벤트 그래프에서 `Try Get Pawn Owner`를 통해 `AMyRemoteCharacter`로 캐스팅한 후, C++에 추가한 `RemoteSpeed` 변수 값을 가져와 애님 그래프 내의 속도 변수(예: `Speed`)에 매 프레임 업데이트합니다.
-   애님 그래프의 상태 머신(State Machine) 또는 블렌드 스페이스(Blend Space)가 이 `Speed` 변수 값을 사용하여 'Idle'과 'Walk/Run' 애니메이션을 자연스럽게 전환하도록 로직을 수정합니다.

---

## 3. 후속 조치 및 문제 해결 (2025-10-11)

### 3.1. 이름표 미표시 문제 진단 및 해결

- **문제 상황:** `AMyRemoteCharacter`를 상속받는 블루프린트에서 이름표가 표시되지 않는 문제 발생.
- **원인 분석:**
    1. C++ 코드 (`AMyRemoteCharacter`, `NameplateWidget`)를 검토한 결과, `NameplateComponent` 생성 및 `SetName` 함수 자체는 정상적으로 구현되어 있었음.
    2. `WebSocketManager`에서 원격 캐릭터의 `SetName` 함수를 호출하는 로직 또한 정상 확인.
    3. 가장 유력한 원인으로, `AMyRemoteCharacter` 블루프린트 내 `NameplateComponent`의 **`Widget Class` 속성이 지정되지 않은 것**으로 진단됨. C++에서 컴포넌트만 생성했을 뿐, 실제로 화면에 표시할 위젯 블루프린트(`WBP_Nameplate`)가 연결되지 않았기 때문에 아무것도 보이지 않는 것이었음.
- **해결 방안:**
    - `AMyRemoteCharacter`를 상속받는 블루프린트(예: `BP_RemoteCharacter`)를 열고, `NameplateComponent`를 선택.
    - Details 패널의 `Widget` 카테고리에서 `Widget Class` 속성을 `WBP_Nameplate`로 설정하도록 안내.

### 3.2. 원격 캐릭터 애니메이션 구현 상세 계획

- **목표:** `WebSocket`을 통해 수신되는 `CurrentSpeed`와 `bIsFalling` 값을 사용하여, 원격 캐릭터의 애니메이션(서기, 걷기/뛰기, 점프/낙하)을 구현.
- **구현 단계 (애니메이션 블루프린트 에디터):**

    1.  **이벤트 그래프 (Event Graph) 설정:**
        - `Event Blueprint Update Animation` 노드에서 시작.
        - `Try Get Pawn Owner` 노드로 캐릭터를 가져온 후, `Cast To AMyRemoteCharacter`로 형 변환.
        - 형 변환에 성공하면, `AMyRemoteCharacter`로부터 `GetCurrentSpeed`와 `Get bIsFalling` 함수를 호출하여 값을 가져옴.
        - 가져온 두 값을 애니메이션 블루프린트 내의 지역 변수 `Speed`와 `IsFalling`에 매 프레임 저장(Set).

    2.  **애님 그래프 (Anim Graph) 설정:**
        - **상태 머신 생성:** `Idle`, `WalkRun`, `Fall`의 3개 상태를 가지는 상태 머신을 생성하고 `Output Pose`에 연결.
        - **상태별 애니메이션 지정:**
            - `Idle` 상태: '서 있기' 애니메이션 재생.
            - `WalkRun` 상태: `Speed` 변수를 사용하는 `Blend Space 1D`를 통해 '걷기/뛰기' 애니메이션 재생.
            - `Fall` 상태: '낙하 루프' 애니메이션 재생.
        - **상태 전이 규칙 설정:**
            - `Idle` -> `WalkRun`: `Speed > 0`
            - `WalkRun` -> `Idle`: `Speed == 0`
            - `Idle` 또는 `WalkRun` -> `Fall`: `IsFalling == true`
            - `Fall` -> `Idle`: `IsFalling == false`

- **최종 조치:** `AMyRemoteCharacter`의 블루프린트에서 `Skeletal Mesh Component`의 `Anim Class`가 위에서 작업한 애니메이션 블루프린트로 지정되었는지 확인.
