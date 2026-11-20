# 데이터 테이블

엑셀(`Tables/Excel/*.xlsx`) → JSON(`Tables/Json/*.json`) → DataTable(`/Game/Data/Tables/DT_*.uasset`) 순서로 만든다. 원본은 엑셀이다. 생성된 JSON과 uasset(`Content/Data/Tables`, `.gitignore`에서 예외로 추적)도 레포에 올린다. 엑셀을 고치면 `Tables\BuildTables.bat`을 실행해 셋을 같이 커밋한다. uasset을 에디터에서 직접 고치면 다음 빌드 때 엑셀 값으로 덮어써진다.

## 실행

```
Tables\BuildTables.bat        :: 엑셀 → JSON → uasset (DebugGame 에디터 바이너리)
Tables\BuildTables.bat dev    :: Development 에디터 바이너리
Tables\BuildTables.bat json   :: 엑셀 → JSON만
```

- uasset 임포트는 커맨드렛이 저장하므로 에디터를 끈 상태에서 한다. 에디터를 켠 채로는 `json`으로 변환한 뒤 에디터 출력 로그(Python)에서 `py "<레포>/Tables/import_tables.py"`를 실행한다.
- 행 구조체가 컴파일되어 있어야 임포트된다. 구조체를 추가·수정했으면 C++ 빌드를 먼저 한다.
- 엑셀 변환은 엔진 내장 Python(표준 라이브러리만 사용)으로 돌리므로 Python이나 openpyxl을 따로 설치할 필요가 없다.

## 엑셀 규칙

- 첫 번째 시트만 읽는다.
- 1행: 열 이름(행 구조체 프로퍼티 이름과 같아야 함). 2행: 설명(무시). 3행부터 데이터.
- 첫 열이 키다. DataTable 행 이름이 되고 같은 이름의 프로퍼티에도 들어간다. 키가 비어 있는 행은 건너뛴다.
- 열 이름이 비었거나 `#`으로 시작하면 무시한다(메모용). `Name`은 행 이름 키라서 열 이름으로 쓸 수 없다.
- 키 중복, 빈 칸은 변환 오류로 멈춘다.

## 새 테이블 추가

1. `Tables/Excel/<이름>.xlsx`를 만든다.
2. `Source/DedicateTPS/Table/TableRows.h`에 `F<이름>Row : FTableRowBase`를 추가한다(파일 이름 + `Row`가 규칙). 빌드한다.
3. `BuildTables.bat`을 실행하면 `/Game/Data/Tables/DT_<이름>`이 생긴다.

등록 코드는 없다. `UTableSubsystem`이 시작할 때 `/Game/Data/Tables` 아래 DataTable을 모두 읽어 행 구조체 타입으로 묶는다. 같은 구조체를 쓰는 테이블이 둘이면 먼저 찾은 것만 쓴다.

## 조회

```cpp
const UTableSubsystem* Tables = UTableSubsystem::Get(this);
const FWeaponTableRow* Weapon = Tables->Find<FWeaponTableRow>(1);
const FWeaponStatTableRow* Stat = Weapon ? Tables->Find<FWeaponStatTableRow>(Weapon->StatId) : nullptr;

// 한 줄로
const FWeaponTableRow* M4 = UTableSubsystem::FindRow<FWeaponTableRow>(this, 1);
```

키는 `LexToString`으로 행 이름이 되는 타입이면 된다(`int32`, `FName`, `FString`). 없으면 `nullptr`과 로그가 남는다. 반환 포인터는 GameInstance가 살아 있는 동안 유효하다. 경기 재시작(`?Restart`)은 GameInstance를 유지한다.

## 현재 테이블

| 테이블 | 키 | 열 |
|---|---|---|
| `WeaponTable` | `Id` | `WeaponName`, `StatId`(→ `WeaponStatTable` 키) |
| `WeaponStatTable` | `StatId` | `Damage`, `RecoilPitchMin`, `RecoilPitchMax`, `RecoilYawMax`, `MaxAmmo` |

무기 BP의 `AM4Weapon::WeaponId`가 `WeaponTable` 키다. 무기가 `PostInitializeComponents`에서 `WeaponId → StatId → WeaponStatTable` 순서로 찾아 `Stat`에 복사하고 탄창 크기를 `UAmmoComponent`에 넣는다. 서버와 클라가 같은 테이블을 각자 읽으므로 스탯은 복제하지 않는다. 데미지(서버)와 반동(조작 클라)은 `ATPSCharacter`가 `Weapon->GetStat()`으로 읽는다. 테이블을 못 찾으면 에러 로그를 남기고 스탯이 0이 된다(데미지 0, 반동 없음).

## 함정

- JSON 임포트는 `ReimportDataTableFactory`를 쓴다. 부모인 `CSVImportFactory`는 `FactoryCanImport`가 `.csv`만 허용해서, 지정해도 무시되고 커맨드렛에서는 알림 창 생성 중 크래시한다.
- `pythonscript` 커맨드렛은 실패 시 `-1`을 반환한다. 배치에서 `if errorlevel 1`은 음수를 못 잡으므로 `%errorlevel% neq 0`으로 검사한다.
- 테이블은 참조 없이 경로로 읽으므로 `DefaultGame.ini`의 `DirectoriesToAlwaysCook`에 `/Game/Data/Tables`를 넣어 쿠킹에 포함한다. 폴더를 바꾸면 이 설정, `UTableSubsystem::TablePath`, `import_tables.py`의 `DEST_PATH`를 함께 바꾼다.
