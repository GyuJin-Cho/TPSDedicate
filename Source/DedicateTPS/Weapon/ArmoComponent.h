#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArmoComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnArmoChanged, int32, Armo, int32, MaxArmo);

// 무기의 탄약. UE5_CPP_Project에서 이주 (CoreRedirects로 연결)
// - 값 변경은 서버에서만 한다
// - HUD가 필요한 소유 클라이언트에만 복제한다
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DEDICATETPS_API UArmoComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArmoComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure)
	int32 GetArmo() const { return Armo; }

	UFUNCTION(BlueprintPure)
	int32 GetMaxArmo() const { return MaxArmo; }

	// 서버 전용
	void SetArmo(int32 InArmo);

	// 서버 전용. 한 발 차감에 성공하면 true
	bool ConsumeArmo();

	// 소유 클라와 서버에서 호출된다 (HUD 갱신용)
	UPROPERTY(BlueprintAssignable)
	FOnArmoChanged OnArmoChanged;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Armo();

	UPROPERTY(EditDefaultsOnly, Category = "Armo", meta = (ClampMin = "1"))
	int32 MaxArmo = 30;

	UPROPERTY(ReplicatedUsing = OnRep_Armo)
	int32 Armo = 30;
};