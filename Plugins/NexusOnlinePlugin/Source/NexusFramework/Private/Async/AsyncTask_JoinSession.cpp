#include "Async/AsyncTask_JoinSession.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "GameFramework/PlayerController.h"
#include "Utils/NexusOnlineHelpers.h"

#define LOCTEXT_NAMESPACE "NexusOnline|JoinSession"

// ──────────────────────────────────────────────
// Create & configure async node
// ──────────────────────────────────────────────
UAsyncTask_JoinSession* UAsyncTask_JoinSession::JoinSession( UObject* WorldContextObject, const FOnlineSessionSearchResultData& SessionResult, ENexusSessionType SessionType)
{
	UAsyncTask_JoinSession* Node = NewObject<UAsyncTask_JoinSession>();
	Node->WorldContextObject = WorldContextObject;
	Node->RawResult = SessionResult.RawResult;
	Node->DesiredType = SessionType;

	return Node;
}

// ──────────────────────────────────────────────
// Start the join operation
// ──────────────────────────────────────────────
void UAsyncTask_JoinSession::Activate()
{
	if (!WorldContextObject)
	{
		UE_LOG(LogTemp, Error, TEXT("[JoinSession] Invalid WorldContextObject."));
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
		UE_LOG(LogTemp, Error, TEXT("[JoinSession] Online session interface invalid."));
		OnFailure.Broadcast();
		
		return;
	}

	const FName InternalSessionName = NexusOnline::SessionTypeToName(DesiredType);

	// Check available slots (public/private)
	const int32 MaxPublicConnections = RawResult.Session.SessionSettings.NumPublicConnections;
	const int32 MaxPrivateConnections = RawResult.Session.SessionSettings.NumPrivateConnections;
	const int32 OpenPublicConnections = RawResult.Session.NumOpenPublicConnections;
	const int32 OpenPrivateConnections = RawResult.Session.NumOpenPrivateConnections;

	const bool bHasOpenPublicConnections = OpenPublicConnections > 0;
	const bool bHasOpenPrivateConnections = OpenPrivateConnections > 0;

	if (!bHasOpenPublicConnections && !bHasOpenPrivateConnections)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[JoinSession] Session '%s' is full (%d/%d players)."),
			*InternalSessionName.ToString(),
			(MaxPublicConnections - OpenPublicConnections) + (MaxPrivateConnections - OpenPrivateConnections),
			MaxPublicConnections + MaxPrivateConnections);

		OnFailure.Broadcast();
		return;
	}

	JoinDelegateHandle = Session->AddOnJoinSessionCompleteDelegate_Handle
	(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UAsyncTask_JoinSession::OnJoinSessionComplete)
	);

	UE_LOG(LogTemp, Log, TEXT("[JoinSession] Attempting to join session type '%s'..."), *InternalSessionName.ToString());

	if (!Session->JoinSession(0, InternalSessionName, RawResult))
	{
		UE_LOG(LogTemp, Error, TEXT("[JoinSession] JoinSession() failed to start."));
		OnFailure.Broadcast();
	}
}

// ──────────────────────────────────────────────
// 📩 Callback : join result
// ──────────────────────────────────────────────
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
		UE_LOG(LogTemp, Error, TEXT("[JoinSession] Failed with code %d."), static_cast<int32>(Result));
		OnFailure.Broadcast();
		
		return;
	}

	FString ConnectString;
	if (!Session->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogTemp, Error, TEXT("[JoinSession] Failed to resolve connect string."));
		OnFailure.Broadcast();
		
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[JoinSession] Connecting to: %s"), *ConnectString);

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		PC->ClientTravel(ConnectString, TRAVEL_Absolute);
	}

	OnSuccess.Broadcast();
}

#undef LOCTEXT_NAMESPACE
