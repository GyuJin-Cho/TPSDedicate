#include "TPSGameMode.h"
#include "TPSGameState.h"
#include "TPSPlayerState.h"
#include "TPSPlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "TPSPlayerController.h"
#include "DedicateTPS/Utill/CHelpers.h"
#include "DedicateTPS/Utill/Utill.h"

ATPSGameMode::ATPSGameMode()
{
	CHelpers::GetClass<APawn>(&DefaultPawnClass, TEXT("/Game/Player/TPSCharacter_BP.TPSCharacter_BP_C"));
	PlayerStateClass = ATPSPlayerState::StaticClass();
	GameStateClass = ATPSGameState::StaticClass();
	PlayerControllerClass = ATPSPlayerController::StaticClass();
}

void ATPSGameMode::InitGameState()
{
	Super::InitGameState();

	if (ATPSGameState* GS = GetGameState<ATPSGameState>())
	{
		GS->SetRequiredPlayers(RequiredPlayers);
	}
}

void ATPSGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	// ErrorMessage가 비어있지 않으면 접속 거부
	if (!ErrorMessage.IsEmpty()) return;

	ErrorMessage = GetJoinRejectReason();
	if (!ErrorMessage.IsEmpty())
	{
		WARNING_LOG(TEXT("[Server] Reject %s : %s"), *Address, *ErrorMessage);
	}
}

APlayerController* ATPSGameMode::Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	// PreLogin을 통과하고 맵을 로딩하는 동안에는 NumPlayers에 잡히지 않으므로 입장 직전에 다시 검사
	ErrorMessage = GetJoinRejectReason();
	if (!ErrorMessage.IsEmpty())
	{
		WARNING_LOG(TEXT("[Server] Reject on login : %s"), *ErrorMessage);
		return nullptr;
	}

	return Super::Login(NewPlayer, InRemoteRole, Portal, Options, UniqueId, ErrorMessage);
}

FString ATPSGameMode::GetJoinRejectReason() const
{
	if (HasMatchStarted()) return TEXT("Match already in progress");
	if (NumPlayers >= RequiredPlayers) return TEXT("Server is full");

	return FString();
}

void ATPSGameMode::PostLogin(APlayerController* NewPlayer)
{
	// Super 안에서 스폰까지 진행될 수 있으므로 먼저 배정
	AssignStartSlot(NewPlayer);

	// Super 안에서 인원이 차면 바로 StartMatch까지 진행됨
	Super::PostLogin(NewPlayer);

	const FString Name = NewPlayer->PlayerState ? NewPlayer->PlayerState->GetPlayerName() : TEXT("Unknown");
	INFO_LOG(TEXT("[Server] Join : %s (%d/%d)"), *Name, NumPlayers, RequiredPlayers);
}

void ATPSGameMode::Logout(AController* Exiting)
{
	const FString Name = Exiting && Exiting->PlayerState ? Exiting->PlayerState->GetPlayerName() : TEXT("Unknown");

	Super::Logout(Exiting);

	INFO_LOG(TEXT("[Server] Leave : %s (%d/%d)"), *Name, NumPlayers, RequiredPlayers);

	// 경기 중 인원이 모자라면 경기 종료
	if (IsMatchInProgress() && NumPlayers < RequiredPlayers)
	{
		EndMatch();
	}
}

bool ATPSGameMode::ReadyToStartMatch_Implementation()
{
	return GetMatchState() == MatchState::WaitingToStart && NumPlayers >= RequiredPlayers;
}

void ATPSGameMode::HandleMatchHasStarted()
{
	// 대기 중이던 플레이어 전원 스폰은 Super에서 처리
	Super::HandleMatchHasStarted();

	INFO_LOG(TEXT("[Server] Match started (%d players)"), NumPlayers);
}

void ATPSGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	INFO_LOG(TEXT("[Server] Match ended. Restart in %.1f sec"), RestartDelay);

	// 맵을 다시 불러오면 남은 클라도 함께 이동하고 다시 WaitingToStart부터 시작
	GetWorldTimerManager().SetTimer(RestartTimerHandle, this, &AGameMode::RestartGame, RestartDelay, false);
}

AActor* ATPSGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	const ATPSPlayerState* PS = Player ? Player->GetPlayerState<ATPSPlayerState>() : nullptr;
	const FName AssignedTag = PS && PlayerStartTags.IsValidIndex(PS->GetStartSlot()) ? PlayerStartTags[PS->GetStartSlot()] : NAME_None;
	bool bAssignedStartExists = false;

	// 배정된 PlayerStart가 비어 있으면 사용, 아니면 다른 폰과 겹치지 않는 PlayerStart 중 랜덤 선택
	TArray<APlayerStart*> FreeStarts;
	TArray<APlayerStart*> AllStarts;

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* Start = *It;
		AllStarts.Add(Start);

		const bool bAssignedStart = !AssignedTag.IsNone() && Start->PlayerStartTag == AssignedTag;
		bAssignedStartExists |= bAssignedStart;

		bool bOccupied = false;
		for (TActorIterator<APawn> PawnIt(GetWorld()); PawnIt; ++PawnIt)
		{
			if (FVector::DistSquared(PawnIt->GetActorLocation(), Start->GetActorLocation()) < FMath::Square(150.f))
			{
				bOccupied = true;
				break;
			}
		}

		if (!bOccupied)
		{
			if (bAssignedStart) return Start;

			FreeStarts.Add(Start);
		}
	}

	if (!AssignedTag.IsNone() && !bAssignedStartExists)
	{
		WARNING_LOG(TEXT("[Server] No PlayerStart with tag %s"), *AssignedTag.ToString());
	}

	if (FreeStarts.Num() > 0) return FreeStarts[FMath::RandRange(0, FreeStarts.Num() - 1)];
	if (AllStarts.Num() > 0)  return AllStarts[FMath::RandRange(0, AllStarts.Num() - 1)];

	return Super::ChoosePlayerStart_Implementation(Player);
}

bool ATPSGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	// 접속 시점에 정해 둔 위치는 다른 폰을 고려하지 못하므로 스폰할 때마다 새로 선택
	return false;
}

void ATPSGameMode::AssignStartSlot(APlayerController* NewPlayer)
{
	ATPSPlayerState* NewPS = NewPlayer->GetPlayerState<ATPSPlayerState>();
	if (!NewPS || !GameState) return;

	// 나간 플레이어의 번호는 다음 입장자가 이어받음
	int32 Slot = 0;
	while (GameState->PlayerArray.ContainsByPredicate([Slot](const TObjectPtr<APlayerState>& Other)
	{
		const ATPSPlayerState* OtherPS = Cast<ATPSPlayerState>(Other.Get());
		return OtherPS && OtherPS->GetStartSlot() == Slot;
	}))
	{
		++Slot;
	}

	NewPS->SetStartSlot(Slot);
}

void ATPSGameMode::OnPlayerKilled(AController* Victim, AController* Killer)
{
	if (Victim)
	{
		if (ATPSPlayerState* VictimPS = Victim->GetPlayerState<ATPSPlayerState>())
		{
			VictimPS->AddDeath();
		}
	}

	if (Killer && Killer != Victim)
	{
		if (ATPSPlayerState* KillerPS = Killer->GetPlayerState<ATPSPlayerState>())
		{
			KillerPS->AddKill();
		}
	}

	INFO_LOG(TEXT("[Server] Kill : %s -> %s"),
		Killer && Killer->PlayerState ? *Killer->PlayerState->GetPlayerName() : TEXT("World"),
		Victim && Victim->PlayerState ? *Victim->PlayerState->GetPlayerName() : TEXT("Unknown"));

	if (Victim)
	{
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle,
			FTimerDelegate::CreateUObject(this, &ATPSGameMode::RespawnPlayer, TWeakObjectPtr<AController>(Victim)),
			RespawnDelay, false);
	}
}

void ATPSGameMode::RespawnPlayer(TWeakObjectPtr<AController> Controller)
{
	// 리스폰 대기 중 나갔거나 경기가 끝났으면 무시
	if (!Controller.IsValid() || !IsMatchInProgress()) return;

	if (APawn* OldPawn = Controller->GetPawn())
	{
		Controller->UnPossess();
		OldPawn->Destroy();
	}

	RestartPlayer(Controller.Get());
}