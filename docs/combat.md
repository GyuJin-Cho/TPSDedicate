# 캐릭터·전투

## 캐릭터 (`ATPSCharacter`)

서버 권한 구조다. 클라이언트가 로컬 이펙트를 먼저 재생하고 `ServerFire`를 보내면, 서버가 트레이스와 데미지를 처리한 뒤 `MulticastFireEffects`로 나머지 클라이언트에 전달한다. 체력과 사망은 서버에서만 변경하고 `OnRep`으로 반영하며, 서버에서는 `OnRep`을 직접 호출한다. 사망하면 `GameMode::OnPlayerKilled`가 경기를 끝낸다(리스폰 없음).

애니메이션(`ABP_MainPlayer`, 부모 `UMainPlayerAnimInstance`)은 다른 프로젝트에서 이주했고 `MESH_PC_02` 체형 기준이다. 스켈레톤이 같은 다른 PC 메시로 바꾸면 다리 길이 차이로 캐릭터가 떠 보인다. 이주 전 클래스 경로는 `DefaultEngine.ini`의 `[CoreRedirects]`로 연결한다.

스프린트는 `UTPSCharacterMovementComponent`의 SavedMove 플래그(`FLAG_Custom_0`)로 예측에 포함되고, 다른 클라이언트의 애니메이션용으로 `bReplicatedSprinting`을 복제한다.

C++에서 상속 컴포넌트의 클래스를 바꾸면(`SetDefaultSubobjectClass`) 기존 `TPSCharacter_BP`는 저장된 예전 클래스를 그대로 쓴다. BP 부모 클래스를 `Character`로 바꿨다가 되돌려야 반영된다.

입력은 모두 레거시 `BindAxis`/`BindAction`(`DefaultInput.ini`)이다. Enhanced Input 에셋은 쓰지 않는다.

조준은 `bIsAiming`(COND_SkipOwner 복제)이다. 클라가 로컬에서 바로 바꾸고 `ServerSetAiming`(Reliable)을 보내며, 같은 Reliable 순서라 `ServerFire`보다 먼저 도착한다. 서버는 `ServerFire`에서 `bIsAiming`을 다시 검사한다. 조준 카메라(거리/FOV 보간)는 `Tick`에서 로컬 조종 캐릭터만 처리한다. ABP의 `Aim` 변수는 `UMainPlayerAnimInstance`가 `IsAiming()`으로 채운다.

재장전은 `ReloadMontage`(BP에서 지정)를 재생하고, 탄약은 몽타주의 `Reload Ammo` 노티파이(`UAnimNotify_ReloadAmmo`, 마커 전용) 시점에 서버가 채운다. 서버는 노티파이 발생을 기다리지 않고 몽타주 에셋에서 노티파이 시간을 읽어 타이머로 처리한다(데디 서버 애니메이션 틱은 이동 RPC에 묶여 있음). 재장전 중에는 조준과 사격이 막히고(`bIsReloading`, COND_SkipOwner, 서버도 `ServerFire`에서 검사), 서버는 RPC 지연을 고려해 조금 일찍 재장전을 끝낸다. 끝날 때 우클릭이 눌려 있으면 자동으로 견착한다. ABP는 `Fire || Reload || Aim`일 때만 `UpperBody` 슬롯 분기를 평가하므로, 상체 몽타주를 조준 밖에서 재생하려면 애님 인스턴스의 해당 변수를 채워야 한다.

반동은 `ApplyRecoil`이 사격 직후 ControlRotation에 랜덤 각도를 더하는 방식이다. 카메라가 ControlRotation을 그대로 바라보므로 화면 중앙 크로스헤어(`UMainHudWidget`이 NativePaint로 그림)가 판정 방향과 일치한다. 카메라 셰이크는 화면만 흔들고 판정에는 영향이 없다.

탄이 없을 때 사격하면 `FireOnce`가 `EmptyFireMontage`(BP에서 지정)를 로컬에서만 재생하고 멈춘다. 서버에 보내지 않으므로 다른 클라에는 보이지 않는다. 탄약은 복제값 기준이라 마지막 발 직후 복제가 늦으면 빈 격발 대신 한 발이 더 연출될 수 있다(서버가 거부).

## 전투 구현 시 주의점

- 사격 트레이스는 전용 채널 `COLLISION_WEAPON`(`ECC_GameTraceChannel1`, `DefaultEngine.ini`에서 기본 Block)을 쓴다. 엔진 기본 프로필의 `Pawn`/`CharacterMesh`는 `ECC_Visibility`를 무시하므로 Visibility로 되돌리면 캐릭터에 맞지 않는다. 판정은 캡슐 기준이고 사망 시 캡슐 충돌이 꺼져 시체가 총알을 막지 않는다.
- 무기(`AM4Weapon`, `UAmmoComponent`)는 `UE5_CPP_Project`에서 이주했고 `[CoreRedirects]`로 연결한다. 컴포넌트/프로퍼티 이름을 바꾸면 이주한 BP의 값이 사라진다. 탄약 컴포넌트는 원래 `ArmoComponent`였고, BP에 덮어쓴 값이 없음을 확인한 뒤 `Ammo`로 이름을 바꿨다(리다이렉트 추가). 서버가 `ATPSCharacter::BeginPlay`에서 `WeaponClass`를 스폰해 부착하고, 탄약 차감과 판정은 `ServerFire`가 처리한다. 무기 클래스는 연출만 담당한다.
- 캐릭터 메시 `MESH_PC_02`(PC_01도 동일)에는 PhysicsAsset 참조가 없어 보인다. 메시 기반 판정과 사망 래그돌에 필요하다.
- `ServerFire`의 연사 검증은 평균 속도만 제한한다(`NextServerFireTime` + 여유 시간). 서버 틱에 RPC가 몰려 도착해도 정상 사격이 거부되지 않게 하기 위해서다.
- `LV_Soul_Slum`의 BlockingVolume(기본 프로필 `InvisibleWall`)은 메시보다 높게 솟은 이동 제한용이라 `DefaultEngine.ini`의 `EditProfiles`로 `InvisibleWall`/`InvisibleWallDynamic`의 Weapon 응답을 Ignore로 바꿨다. 총알은 complex 트레이스로 메시에 맞는다.
- 판정은 `TraceShot` 하나로 클라 연출과 서버가 같이 쓴다. 카메라 방향 트레이스로 탄착점을 구한 뒤(시작점은 조준선 위에서 기준점 높이까지 앞으로 옮겨, 카메라와 캐릭터 사이에 선 상대가 맞지 않게 한다) 스프링암 기준점(`GetCameraPivot`) → 총구 → 탄착점 사이가 막혔으면 막힌 곳에 맞는다(코너 피킹 방지). 총구 위치를 쓰므로 서버에서도 메시 본을 갱신한다(`AlwaysTickPoseAndRefreshBones`).
- 부위 데미지(머리 ×5 즉사, 팔다리 ×0.5)는 `TakeDamage`가 포인트 데미지일 때 `GetHitZone`으로 정한다. 메시 충돌(PhysicsAsset)이 없어서, 실제로 맞은 트레이스 방향(`HitInfo.TraceStart`→`TraceEnd`, 2차 검사로 맞으면 카메라 방향과 다르다)으로 총알 경로를 그린 뒤(캡슐 밖으로 나온 조준 팔을 넣으려고 명중 지점보다 캡슐 반지름만큼 앞에서 시작) 본 구간(UE4 마네킹 본 이름)을 반지름을 준 캡슐로 보고 경로가 처음 지나간 부위를 고른다. 머리는 실제로 지나가야 인정하고, 아무 부위도 안 지나간 캡슐 스침은 가장 가까운 몸통/팔다리로 친다. 서버 본 위치를 쓰므로 서버가 몽타주를 재생하지 않는 데서 오는 오차가 그대로 있다. 스켈레톤을 바꾸면 본 이름 표를 고친다. 헤드샷이면 피격자의 `TakeDamage`가 `DamageCauser`(사격자 캐릭터)에 `ClientNotifyHeadshot`을 보내고, `UMainHudWidget`이 `GetLastHeadshotTime()`을 읽어 문구를 그린다. `ServerFire`가 `ApplyPointDamage`의 DamageCauser로 `this`를 넘기는 것을 전제로 한다.
- 피 이펙트는 맞은 캐릭터의 `BloodNiagara`/`BloodParticle`(BP에서 지정)을 쓴다. 사격자는 로컬 `TraceShot` 결과로 먼저 재생하고, 나머지 클라는 `MulticastFireEffects`의 `HitCharacter`로 재생한다. 서버 판정과 다르면 사격자 화면에만 피가 보일 수 있다.
- 서버는 `TraceStart`가 기준점에서 `GetMaxTraceStartDistance()`(긴 팔 길이 + `SocketOffset` + 위치 오차)보다 멀면 거부한다. 벽 너머로 옮긴 시작점은 따로 막지 않아도 `TraceShot`의 총구→탄착점 구간에서 벽에 막힌다.
