#include "MainPlayerAnimInstance.h"
#include "TPSCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetAnimationLibrary.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UMainPlayerAnimInstance::NativeUninitializeAnimation()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	Super::NativeUninitializeAnimation();
}

void UMainPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	ATPSCharacter* Character = Cast<ATPSCharacter>(TryGetPawnOwner());
	if (!Character) return;

	const FVector Velocity = Character->GetVelocity();
	Speed = Velocity.Size2D();

	// 원격 캐릭터는 Controller가 없으므로 ControlRotation 대신 GetBaseAimRotation 사용
	// (Pitch는 RemoteViewPitch로 복제된 값이 들어온다)
	const FRotator AimRotation = Character->GetBaseAimRotation();
	Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, AimRotation);

	Falling = Character->GetCharacterMovement()->IsFalling();
	Die = Character->IsDead();
	Sprint = Character->IsSprinting();
	Aim = Character->IsAiming();

	// ABP는 Fire || Reload || Aim일 때만 UpperBody 슬롯 분기를 쓰므로, 조준을 푼 재장전 중에도 true여야 몽타주가 보인다
	Reload = Character->IsReloading();

	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(AimRotation, Character->GetActorRotation());
	const FRotator Interp = FMath::RInterpTo(FRotator(Pitch, Yaw, 0.f), Delta, DeltaSeconds, 5.f);
	Pitch = FMath::ClampAngle(Interp.Pitch, -90.f, 90.f);
	Yaw = FMath::ClampAngle(Interp.Yaw, -90.f, 90.f);
}
