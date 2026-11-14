#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AmmoComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChanged, int32, Ammo, int32, MaxAmmo);

// 무기의 탄약. UE5_CPP_Project의 ArmoComponent에서 이주하고 이름을 바꿨다 (CoreRedirects로 연결)
// - 값 변경은 서버에서만 한다
// - HUD가 필요한 소유 클라이언트에만 복제한다
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DEDICATETPS_API UAmmoComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAmmoComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure)
	int32 GetAmmo() const { return Ammo; }

	UFUNCTION(BlueprintPure)
	int32 GetMaxAmmo() const { return MaxAmmo; }

	// 서버 전용
	void SetAmmo(int32 InAmmo);

	// 서버 전용. 한 발 차감에 성공하면 true
	bool ConsumeAmmo();

	// 소유 클라와 서버에서 호출된다 (HUD 갱신용)
	UPROPERTY(BlueprintAssignable)
	FOnAmmoChanged OnAmmoChanged;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Ammo();

	UPROPERTY(EditDefaultsOnly, Category = "Ammo", meta = (ClampMin = "1"))
	int32 MaxAmmo = 30;

	UPROPERTY(ReplicatedUsing = OnRep_Ammo)
	int32 Ammo = 30;
};