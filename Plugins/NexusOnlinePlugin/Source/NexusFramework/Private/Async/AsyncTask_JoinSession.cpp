#include "Async/AsyncTask_JoinSession.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "GameFramework/PlayerController.h"
#include "Utils/NexusOnlineHelpers.h"


UAsyncTask_JoinSession* UAsyncTask_JoinSession::JoinSession(UObject* WorldContextObject, const FOnlineSessionSearchResultData& SessionResult, ENexusSessionType SessionType)
{
    UAsyncTask_JoinSession* Node = NewObject<UAsyncTask_JoinSession>();
	Node->WorldContextObject = WorldContextObject;
    Node->RawResult = SessionResult.RawResult;
    Node->DesiredType = SessionType;
	
    return Node;
}

void UAsyncTask_JoinSession::Activate()
{
	if (!WorldContextObject)
	{
		UE_LOG(LogTemp, Error, TEXT("[NexusOnline] Invalid WorldContextObject in JoinSessions"));
		
		OnFailure.Broadcast();
		return;
	}
	
    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
    if (!World)
    {
        OnFailure.Broadcast();
        return;
    }

	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	if (!Session.IsValid())
	{
		OnFailure.Broadcast();
		return;
	}

    const FName InternalSessionName = NexusOnline::SessionTypeToName(DesiredType);

    JoinDelegateHandle = Session->AddOnJoinSessionCompleteDelegate_Handle
	(
        FOnJoinSessionCompleteDelegate::CreateUObject(this, &UAsyncTask_JoinSession::OnJoinSessionComplete)
    );

    UE_LOG(LogTemp, Log, TEXT("[NexusOnline] Joining session type '%s'..."), *InternalSessionName.ToString());

    if (!Session->JoinSession(0, InternalSessionName, RawResult))
    {
        UE_LOG(LogTemp, Error, TEXT("[NexusOnline] JoinSession() failed to start"));
        OnFailure.Broadcast();
    }
}

void UAsyncTask_JoinSession::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
    if (!World)
    {
        OnFailure.Broadcast();
        return;
    }

	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	if (!Session.IsValid())
	{
		OnFailure.Broadcast();
		return;
	}
	
	Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinDelegateHandle);

    if (Result != EOnJoinSessionCompleteResult::Success)
    {
        UE_LOG(LogTemp, Error, TEXT("[NexusOnline] Join session failed (%d)"), static_cast<int32>(Result));
        OnFailure.Broadcast();
        return;
    }

    FString ConnectString;
    if (!Session->GetResolvedConnectString(SessionName, ConnectString))
    {
        UE_LOG(LogTemp, Error, TEXT("[NexusOnline] Failed to resolve connection string"));
        OnFailure.Broadcast();
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[NexusOnline] Connecting to: %s"), *ConnectString);

    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        PC->ClientTravel(ConnectString, TRAVEL_Absolute);
    }

    OnSuccess.Broadcast();
}