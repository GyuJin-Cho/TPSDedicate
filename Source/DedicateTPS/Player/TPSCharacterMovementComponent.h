#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TPSCharacterMovementComponent.generated.h"

// 스프린트를 이동 예측에 포함시키는 CMC
// - 클라: bWantsToSprint를 SavedMove의 FLAG_Custom_0으로 서버에 전송
// - 서버: 같은 플래그로 이동을 재현하므로 속도 차이로 인한 위치 보정이 생기지 않는다
UCLASS()
class DEDICATETPS_API UTPSCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprint")
	float SprintSpeed = 600.f;

	// 입력 상태 (로컬 클라와 서버에서만 유효)
	bool bWantsToSprint = false;

	// 실제로 스프린트 속도가 적용되는 상태
	bool IsSprinting() const;

	virtual float GetMaxSpeed() const override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

protected:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
};

class FSavedMove_TPSCharacter : public FSavedMove_Character
{
	using Super = FSavedMove_Character;

public:
	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, const FVector& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* C) override;

private:
	bool bSavedWantsToSprint = false;
};

class FNetworkPredictionData_Client_TPSCharacter : public FNetworkPredictionData_Client_Character
{
	using Super = FNetworkPredictionData_Client_Character;

public:
	explicit FNetworkPredictionData_Client_TPSCharacter(const UCharacterMovementComponent& ClientMovement);
	virtual FSavedMovePtr AllocateNewMove() override;
};
