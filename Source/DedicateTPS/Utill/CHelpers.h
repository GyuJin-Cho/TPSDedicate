#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"

class DEDICATETPS_API CHelpers
{
public:
	// 에셋을 가져온다. 생성자에서만 사용 가능
	template<typename T>
	static void GetAsset(T** OutObject, FString InPath)
	{
		ConstructorHelpers::FObjectFinder<T> asset(*InPath);
		verifyf(asset.Succeeded(), TEXT("Asset Not Found : %s"), *InPath);

		*OutObject = asset.Object;
	}

	template<typename T>
	static void GetAsset(TObjectPtr<T>* OutObject, FString InPath)
	{
		T* object = nullptr;
		GetAsset(&object, InPath);
		*OutObject = object;
	}

	// 경로로 에셋을 동적 로드한다. 런타임에 사용
	template<typename T>
	static void GetAssetDynamic(T** OutObject, FString InPath)
	{
		T* obj = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *InPath));
		verifyf(!!obj, TEXT("Asset Not Found(Dynamic) : %s"), *InPath);

		*OutObject = obj;
	}

	// 클래스(BP 등)를 가져온다. 생성자에서만 사용 가능
	template<typename T>
	static void GetClass(TSubclassOf<T>* OutClass, FString InPath)
	{
		ConstructorHelpers::FClassFinder<T> asset(*InPath);
		verifyf(asset.Succeeded(), TEXT("Class Not Found : %s"), *InPath);

		*OutClass = asset.Class;
	}

	// 씬 컴포넌트를 생성한다. 부모가 없으면 루트로 지정
	template<typename T>
	static void CreateComponent(AActor* InActor, T** OutComponent, FName InName, USceneComponent* InParent = nullptr)
	{
		*OutComponent = InActor->CreateDefaultSubobject<T>(InName);

		if (!!InParent)
		{
			(*OutComponent)->SetupAttachment(InParent);
			return;
		}

		InActor->SetRootComponent(*OutComponent);
	}

	template<typename T>
	static void CreateComponent(AActor* InActor, TObjectPtr<T>* OutComponent, FName InName, USceneComponent* InParent = nullptr)
	{
		T* component = nullptr;
		CreateComponent(InActor, &component, InName, InParent);
		*OutComponent = component;
	}

	// 씬 컴포넌트를 생성해 부모의 소켓에 붙인다
	template<typename T>
	static void CreateComponentSocket(AActor* InActor, T** OutComponent, FName InName, FName InSocketName, USceneComponent* InParent = nullptr)
	{
		*OutComponent = InActor->CreateDefaultSubobject<T>(InName);

		if (!!InParent)
		{
			(*OutComponent)->SetupAttachment(InParent, InSocketName);
			return;
		}

		InActor->SetRootComponent(*OutComponent);
	}

	template<typename T>
	static void CreateComponentSocket(AActor* InActor, TObjectPtr<T>* OutComponent, FName InName, FName InSocketName, USceneComponent* InParent = nullptr)
	{
		T* component = nullptr;
		CreateComponentSocket(InActor, &component, InName, InSocketName, InParent);
		*OutComponent = component;
	}

	// 액터 컴포넌트를 생성한다
	template<typename T>
	static void CreateActorComponent(AActor* InActor, T** OutComponent, FName InName)
	{
		*OutComponent = InActor->CreateDefaultSubobject<T>(InName);
	}

	template<typename T>
	static void CreateActorComponent(AActor* InActor, TObjectPtr<T>* OutComponent, FName InName)
	{
		T* component = nullptr;
		CreateActorComponent(InActor, &component, InName);
		*OutComponent = component;
	}

	// 액터에서 컴포넌트를 찾는다
	template<typename T>
	static T* GetComponent(AActor* InActor)
	{
		return Cast<T>(InActor->GetComponentByClass(T::StaticClass()));
	}
};
