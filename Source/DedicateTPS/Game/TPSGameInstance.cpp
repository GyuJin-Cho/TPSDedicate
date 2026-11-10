#include "TPSGameInstance.h"
#include "Engine/Engine.h"
#include "DedicateTPS/Utill/Utill.h"

void UTPSGameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UTPSGameInstance::HandleNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &UTPSGameInstance::HandleTravelFailure);
	}
}

void UTPSGameInstance::Shutdown()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnTravelFailure().RemoveAll(this);
	}

	Super::Shutdown();
}

FString UTPSGameInstance::ConsumeLastConnectError()
{
	FString Result = MoveTemp(LastConnectError);
	LastConnectError.Reset();
	return Result;
}

void UTPSGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& Error)
{
	WARNING_LOG(TEXT("[Client] Network failure (%s) : %s"), ENetworkFailure::ToString(FailureType), *Error);
	LastConnectError = Error.IsEmpty() ? FString(ENetworkFailure::ToString(FailureType)) : Error;
	OnConnectFailed.Broadcast(LastConnectError);
}

void UTPSGameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error)
{
	WARNING_LOG(TEXT("[Client] Travel failure (%s) : %s"), ETravelFailure::ToString(FailureType), *Error);
	LastConnectError = Error.IsEmpty() ? FString(ETravelFailure::ToString(FailureType)) : Error;
	OnConnectFailed.Broadcast(LastConnectError);
}