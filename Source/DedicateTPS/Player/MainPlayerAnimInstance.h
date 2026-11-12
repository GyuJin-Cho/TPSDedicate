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

	// 아래 3개는 ATPSCharacter에 아직 기능이 없어 항상 false (ABP 참조 유지용)
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool Aim;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool Fire;

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
