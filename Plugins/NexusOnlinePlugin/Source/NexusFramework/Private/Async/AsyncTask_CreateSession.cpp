#include "Async/AsyncTask_CreateSession.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Utils/NexusOnlineHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"


UAsyncTask_CreateSession* UAsyncTask_CreateSession::CreateSession(UObject* WorldContextObject, const FSessionSettingsData& SettingsData)
{
    UAsyncTask_CreateSession* Node = NewObject<UAsyncTask_CreateSession>();
    Node->WorldContextObject = WorldContextObject;
    Node->Data = SettingsData;
	
    return Node;
}

void UAsyncTask_CreateSession::Activate()
{
    if (!WorldContextObject)
    {
        UE_LOG(LogTemp, Error, TEXT("[NexusOnline] Invalid WorldContextObject in CreateSession"));
    	
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

    const FName InternalSessionName = NexusOnline::SessionTypeToName(Data.SessionType);
    if (Session->GetNamedSession(InternalSessionName))
    {
        UE_LOG(LogTemp, Warning, TEXT("[NexusOnline] Existing %s found, destroying before recreation."), *InternalSessionName.ToString());
        Session->DestroySession(InternalSessionName);
    }

    FOnlineSessionSettings Settings;
    Settings.bIsLANMatch = Data.bIsLAN;
    Settings.bShouldAdvertise = !Data.bIsPrivate;
    Settings.bUsesPresence = true;
    Settings.bAllowJoinInProgress = true;
    Settings.NumPublicConnections = Data.MaxPlayers;

    Settings.Set("SESSION_DISPLAY_NAME", Data.SessionName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set("MAP_NAME_KEY", Data.MapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set("GAME_MODE_KEY", Data.GameMode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set("SESSION_TYPE_KEY", NexusOnline::SessionTypeToName(Data.SessionType).ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set("USES_PRESENCE", true, EOnlineDataAdvertisementType::ViaOnlineService);

    CreateDelegateHandle = Session->AddOnCreateSessionCompleteDelegate_Handle
	(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &UAsyncTask_CreateSession::OnCreateSessionComplete)
    );

    UE_LOG(LogTemp, Log, TEXT("[NexusOnline] Creating %s (Display: %s)"), *InternalSessionName.ToString(), *Data.SessionName);
    Session->CreateSession(0, InternalSessionName, Settings);
}

void UAsyncTask_CreateSession::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);

    if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World))
    {
        Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateDelegateHandle);
    }

    UE_LOG(LogTemp, Log, TEXT("[NexusOnline] Session '%s' creation result: %s"),
    	*SessionName.ToString(), bWasSuccessful ? TEXT("SUCCESS") : TEXT("FAILURE"));

    if (!bWasSuccessful)
    {
        OnFailure.Broadcast();
        return;
    }

    OnSuccess.Broadcast();
    UGameplayStatics::OpenLevel(World, FName(*Data.MapName), true, TEXT("listen"));
}
