# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 프로젝트 개요

Unreal Engine 5.8(소스 빌드, `G:\DediCateProject\UnrealEngine5.8`) 기반의 데디케이티드 서버 2인 TPS. 런타임 모듈은 `DedicateTPS` 하나다.

## 작업 규칙

- 엔진 API는 기억에 의존하지 말고 `G:\DediCateProject\UnrealEngine5.8` 소스에서 시그니처를 확인한다.
- 코드를 수정해 CLAUDE.md나 `docs/`의 설명이 틀려지면 같은 작업에서 문서도 갱신한다.
- CLAUDE.md에는 코드만 봐서는 알 수 없는 것(명령어, 함정, 결정 이유)만 적는다. 코드를 읽으면 알 수 있는 구조 설명은 적지 않는다.

## 명령어

C++를 수정한 뒤 빌드한다 (에디터를 끈 상태에서):

```
G:\DediCateProject\UnrealEngine5.8\Engine\Build\BatchFiles\Build.bat DedicateTPSEditor Win64 DebugGame -Project=G:\DediCateProject\DedicateTPS\DedicateTPS.uproject -WaitMutex -NoHotReload
```

서버 실행 (에디터 바이너리를 `-server`로 띄움, 포트 7777, `RunServer.bat dev`는 Development 빌드 사용):

```
RunServer.bat
```

화면 없는 클라이언트로 접속 테스트:

```
UnrealEditor-Win64-DebugGame.exe <uproject 경로> 127.0.0.1:7777 -game -nullrhi -unattended -NoSound -abslog=<로그 경로>
```

- Git Bash에서 실행할 때는 `MSYS_NO_PATHCONV=1`을 설정한다. 없으면 `/Game/...` 맵 경로가 `C:/Program Files/Git/Game/...`으로 변환돼 서버가 바로 종료된다.
- PIE를 멈추면 모든 클라이언트가 동시에 끊긴다. "한 명만 나감" 시나리오는 클라이언트를 별도 프로세스로 띄워 테스트한다.
- 린트와 자동화 테스트는 없다. 동작 검증은 위 방식으로 실제 접속한 뒤 로그를 확인한다.
- 로그는 `Saved/Logs`에 쌓이며, 게임 로그는 `[Server]`, `[Client]` 접두사로 찾는다.
- `DedicateTPSServer.exe`(Server 타깃)는 쿠킹된 콘텐츠가 있어야 실행된다. 쿠킹 없이 테스트할 때는 `RunServer.bat`을 쓴다.

## 아키텍처

### 매치 수명주기 (`ATPSGameMode`, `AGameMode` 상속)

`WaitingToStart`(인원 대기) → `InProgress`(`NumPlayers >= RequiredPlayers`) → 첫 킬(`OnPlayerKilled`) 또는 경기 중 인원 부족 시 `EndMatch` → `RestartDelay` 후 `RestartGame`.

승패는 첫 킬로 정하고 경기 중 리스폰은 없다. 승자는 `ATPSGameState::WinnerPlayerId`로 복제되고(킬러가 없거나 상대가 나가면 남은 쪽), `UMatchStatusWidget`이 `WaitingPostMatch`에서 "승리"/"패배"를 표시한다. PlayerState 포인터가 아니라 PlayerId인 이유는 승자가 종료 후 나가도 패자 표시가 유지되게 하기 위해서다. 승자가 없을 때만 기존 종료 안내가 나온다. 낙사(`FellOutOfWorld`)도 `Die(nullptr)`로 처리해 경기가 끝난다(엔진 기본은 `Die` 없이 `Destroy`).

재시작은 non-seamless `?Restart` 트래블이다. 클라이언트가 재접속하고 GameMode, GameState, PlayerState가 모두 새로 만들어지므로, 경기 간에 유지할 상태는 이 객체들에 둘 수 없다.

### 입장 검사

`PreLogin`과 `Login` 양쪽에서 `GetJoinRejectReason()`을 호출한다. `PreLogin`은 클라이언트가 맵을 로딩하기 전에 실행되고, 로딩 중인 클라이언트는 `NumPlayers`에 잡히지 않아서 `Login`에서 다시 검사해야 한다. 입장 조건을 바꿀 때는 이 함수만 고친다.

### 접속 실패 표시

`UTPSGameInstance`가 엔진의 네트워크/트래블 실패를 받아 사유를 저장한다. 패키지/Standalone에서는 실패 시 메뉴 맵이 다시 로드되므로 `UConnectWidget::NativeConstruct`가 저장된 사유를 꺼내 표시하고, PIE에서는 맵이 다시 로드되지 않으므로 `OnConnectFailed` 델리게이트로 바로 알린다. 두 경로를 모두 유지해야 한다.

### 맵과 GameMode 연결

서버 맵(`LV_Soul_Slum`)은 `DefaultEngine.ini`의 전역 기본 GameMode로 `ATPSGameMode`를 쓰고, `MainMenu`는 월드 세팅 오버라이드로 `MenuGameMode_BP`를 쓴다.

### 스폰

`AssignStartSlot`이 입장 시 비어 있는 가장 작은 번호를 PlayerState의 `StartSlot`(서버 전용, 비복제)에 넣고, `ChoosePlayerStart`가 이를 `PlayerStartTags`로 변환한다. 해당 PlayerStart가 막혀 있으면 빈 PlayerStart 중에서 랜덤으로 고른다. 맵의 PlayerStart 태그는 `PlayerStartTags` 기본값(`PlayerStart`, `PlayerStart2`)과 일치해야 한다.

### 캐릭터·전투

캐릭터·전투·사격·재장전 코드를 수정하기 전에 `docs/combat.md`를 먼저 읽는다.

### 알려진 이슈

코드 리뷰 전에 `docs/known-issues.md`를 읽고, 거기 있는 항목은 새 문제로 다루지 않는다.

### 코드 스타일

- 헤더 상단에 `class UFoo;` 전방선언을 모아 쓰지 않는다. 사용 위치에 `class`를 붙인다 (상세 형식 지정자): `TObjectPtr<class UFoo>`, `TSubclassOf<class UFoo>`, `class UFoo* GetFoo()`.
- 이 방식은 해당 타입이 **처음 나오는 줄**에 `class`가 있어야 컴파일된다. 함수 반환형이 멤버 변수보다 위에 있으면 반환형에도 붙인다.

### 헬퍼

- `CHelpers::GetAsset`/`GetClass`는 생성자 전용이며 경로가 틀리면 `verifyf`로 크래시한다. 에셋을 옮기면 코드의 경로도 함께 수정한다.
- `Utill.h`의 `*_LOG`는 `LogTemp`로 출력한다. `*_MSG`는 모달 대화상자라서 서버 코드에서 쓰면 안 된다.
- 폴더명은 `Utill`이 맞다 (include 경로가 이 철자를 사용).

## 저장소

GitHub: `GyuJin-Cho/TPSDedicate` (public). 소스 전용 레포이고 Git LFS는 쓰지 않는다. `Content`는 레포에 없으므로 BP와 맵 변경은 버전 관리되지 않는다.

`.gitignore`는 화이트리스트 방식이다. 루트에 새 파일이나 폴더를 추가해 추적하려면 `.gitignore`에 `!/이름`을 추가해야 한다.

`CLAUDE.md`, `README.md`, `docs/`는 추적한다. 레포가 public이므로 이 문서들에 개인정보나 비공개로 둘 내용을 적지 않는다.
