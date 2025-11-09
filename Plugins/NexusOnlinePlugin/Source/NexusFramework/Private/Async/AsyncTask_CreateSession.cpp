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
        UE_LOG(LogTemp, Warning, TEXT("[NexusOnline] Null WorldContextObject in CreateSession, attempting fallback world."));
    }

    UWorld* World = GetWorldSafe();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("[NexusOnline] Unable to resolve a valid world for CreateSession."));
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
    UWorld* World = GetWorldSafe();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("[NexusOnline] World invalid on OnCreateSessionComplete."));
        OnFailure.Broadcast();
        return;
    }

    IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
    if (Session.IsValid())
    {
        Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateDelegateHandle);
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("[NexusOnline] ❌ SessionInterface invalid!"));
        return;
    }

    const FString Status = FString::Printf(TEXT("Session '%s' → %s"),
        *SessionName.ToString(), bWasSuccessful ? TEXT("✅ SUCCESS") : TEXT("❌ FAILURE"));

    GEngine->AddOnScreenDebugMessage(-1, 5.f,
        bWasSuccessful ? FColor::Green : FColor::Red, Status);

    // Subsystem actif
    if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
    {
        FString SubsystemName = OSS->GetSubsystemName().ToString();
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
            FString::Printf(TEXT("[Subsystem] %s"), *SubsystemName));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("[Subsystem] ❌ None (Steam not initialized?)"));
    }

    // Vérif NetDriver
    if (World && World->GetNetDriver())
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Silver,
            FString::Printf(TEXT("[NetDriver] %s"), *World->GetNetDriver()->GetClass()->GetName()));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange,
            TEXT("[NetDriver] ⚠️ None or invalid!"));
    }

    // Vérif session
    if (Session.IsValid())
    {
        FNamedOnlineSession* Named = Session->GetNamedSession(SessionName);
        if (Named)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
                FString::Printf(TEXT("[Session] Found (%d / %d players)"),
                    Named->NumOpenPrivateConnections,
                    Named->SessionSettings.NumPublicConnections));
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
                TEXT("[Session] Not found after creation!"));
        }
    }

    if (!bWasSuccessful)
    {
        // Erreur détaillée
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("❌ CreateSession failed! Possible causes:"));
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT(" - Steam not initialized (check steam_appid.txt)"));
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT(" - Invalid world or subsystem not ready"));
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT(" - Wrong NetDriver or network config"));

        OnFailure.Broadcast();
        return;
    }

    // Succès
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
        FString::Printf(TEXT("✅ Session '%s' created, opening '%s' (listen)"),
            *SessionName.ToString(), *Data.MapName));

    OnSuccess.Broadcast();
    UGameplayStatics::OpenLevel(World, FName(*Data.MapName), true, TEXT("listen"));
}

UWorld* UAsyncTask_CreateSession::GetWorldSafe()
{
    if (CachedWorld.IsValid())
    {
        return CachedWorld.Get();
    }

    if (UWorld* World = NexusOnline::ResolveWorld(WorldContextObject))
    {
        CachedWorld = World;
        return World;
    }

    return nullptr;
}

