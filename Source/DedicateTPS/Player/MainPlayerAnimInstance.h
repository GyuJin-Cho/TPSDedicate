// 캐릭터 ABP의 부모 클래스 (UE5_CPP_Project에서 이식)
// 클래스명을 바꾸면 기존 ABP의 부모 연결과 DefaultEngine.ini의 CoreRedirects가 깨지므로 유지한다.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MainPlayerAnimInstance.generated.h"

UCLASS()
class DEDICATETPS_API UMainPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float Speed;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float Direction;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool Falling;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool Sprint;

	// ATPSCharacter::IsAiming(). 원격 캐릭터는 복제된 값
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool Aim;

	// ATPSCharacter에 대응 기능이 없어 항상 false (ABP 참조 유지용)
	// 사격 동작은 ATPSCharacter::FireMontage(Slot 노드)로 재생하고, 사격은 조준 중에만 되므로 Aim으로 충분하다
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool Fire;

	// ATPSCharacter::IsReloading(). ABP의 UpperBody 슬롯 분기 조건(Fire || Reload || Aim)에 쓰인다
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool Reload;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool Die;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float Pitch;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float Yaw;

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// ABP의 BasicFootIKTimer 등 이 인스턴스의 타이머가 메시 파괴 후에도 돌지 않도록 정리
	virtual void NativeUninitializeAnimation() override;};
