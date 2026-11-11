#include "TPSCharacterMovementComponent.h"
#include "TPSCharacter.h"

bool UTPSCharacterMovementComponent::IsSprinting() const
{
	return bWantsToSprint && IsMovingOnGround() && !IsCrouching();
}

float UTPSCharacterMovementComponent::GetMaxSpeed() const
{
	if (IsSprinting())
	{
		return SprintSpeed;
	}
	return Super::GetMaxSpeed();
}

void UTPSCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

void UTPSCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	// 시뮬레이티드 프록시(다른 클라에 보이는 캐릭터)의 애니메이션용으로 서버가 상태를 복제
	if (CharacterOwner && CharacterOwner->HasAuthority())
	{
		if (ATPSCharacter* TPSCharacter = Cast<ATPSCharacter>(CharacterOwner))
		{
			TPSCharacter->SetReplicatedSprinting(IsSprinting());
		}
	}
}

FNetworkPredictionData_Client* UTPSCharacterMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		UTPSCharacterMovementComponent* MutableThis = const_cast<UTPSCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_TPSCharacter(*this);
	}
	return ClientPredictionData;
}

// ---------------------------------------------------------------- SavedMove

void FSavedMove_TPSCharacter::Clear()
{
	Super::Clear();
	bSavedWantsToSprint = false;
}

uint8 FSavedMove_TPSCharacter::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if (bSavedWantsToSprint)
	{
		Result |= FLAG_Custom_0;
	}
	return Result;
}

bool FSavedMove_TPSCharacter::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	if (bSavedWantsToSprint != static_cast<FSavedMove_TPSCharacter*>(NewMove.Get())->bSavedWantsToSprint)
	{
		return false;
	}
	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_TPSCharacter::SetMoveFor(ACharacter* C, float InDeltaTime, const FVector& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	if (const UTPSCharacterMovementComponent* CMC = Cast<UTPSCharacterMovementComponent>(C->GetCharacterMovement()))
	{
		bSavedWantsToSprint = CMC->bWantsToSprint;
	}
}

void FSavedMove_TPSCharacter::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);

	if (UTPSCharacterMovementComponent* CMC = Cast<UTPSCharacterMovementComponent>(C->GetCharacterMovement()))
	{
		CMC->bWantsToSprint = bSavedWantsToSprint;
	}
}

FNetworkPredictionData_Client_TPSCharacter::FNetworkPredictionData_Client_TPSCharacter(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_TPSCharacter::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_TPSCharacter());
}
