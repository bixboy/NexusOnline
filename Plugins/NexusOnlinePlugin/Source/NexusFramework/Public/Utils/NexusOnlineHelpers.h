#pragma once
#include "CoreMinimal.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Types/OnlineSessionData.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

namespace NexusOnline
{
	//───────────────────────────────────────────────
	// 🔹 ENUM ↔ NAME Conversion
	//───────────────────────────────────────────────
	static FORCEINLINE FName SessionTypeToName(ENexusSessionType Type)
	{
		switch (Type)
		{
		case ENexusSessionType::PartySession: return FName("PartySession");
		case ENexusSessionType::SpectatorSession: return FName("SpectatorSession");
		case ENexusSessionType::CustomSession: return FName("CustomSession");
		default: return FName("GameSession");
		}
	}

	static FORCEINLINE ENexusSessionType NameToSessionType(const FName& Name)
	{
		if (Name == "PartySession") return ENexusSessionType::PartySession;
		if (Name == "SpectatorSession") return ENexusSessionType::SpectatorSession;
		if (Name == "CustomSession") return ENexusSessionType::CustomSession;
		return ENexusSessionType::GameSession;
	}

	//───────────────────────────────────────────────
	// 🔹 Subsystem Access Helpers
	//───────────────────────────────────────────────
	static FORCEINLINE IOnlineSubsystem* GetSubsystem(UWorld* World)
	{
		return Online::GetSubsystem(World);
	}

	static FORCEINLINE bool IsSubsystemAvailable(UWorld* World)
	{
		return (GetSubsystem(World) != nullptr);
	}

	//───────────────────────────────────────────────
	// 🔹 Interface Accessors
	//───────────────────────────────────────────────
	static FORCEINLINE IOnlineSessionPtr GetSessionInterface(UWorld* World)
	{
		if (IOnlineSubsystem* Subsystem = GetSubsystem(World))
			return Subsystem->GetSessionInterface();
		
		return nullptr;
	}

	static FORCEINLINE IOnlineIdentityPtr GetIdentityInterface(UWorld* World)
	{
		if (IOnlineSubsystem* Subsystem = GetSubsystem(World))
			return Subsystem->GetIdentityInterface();
		
		return nullptr;
	}

	static FORCEINLINE IOnlineFriendsPtr GetFriendsInterface(UWorld* World)
	{
		if (IOnlineSubsystem* Subsystem = GetSubsystem(World))
			return Subsystem->GetFriendsInterface();
		
		return nullptr;
	}

        static FORCEINLINE IOnlineExternalUIPtr GetExternalUIInterface(UWorld* World)
        {
                if (IOnlineSubsystem* Subsystem = GetSubsystem(World))
                        return Subsystem->GetExternalUIInterface();

                return nullptr;
        }

        //───────────────────────────────────────────────
        // 🔹 World resolution helper (handles standalone mode)
        //───────────────────────────────────────────────
        static FORCEINLINE UWorld* ResolveWorld(UObject* WorldContextObject)
        {
                auto IsPreferredWorldType = [](EWorldType::Type Type)
                {
                        switch (Type)
                        {
                        case EWorldType::Game:
                        case EWorldType::PIE:
                        case EWorldType::GamePreview:
                        case EWorldType::GameRPC:
                                return true;
                        default:
                                return false;
                        }
                };

                if (GEngine)
                {
                        if (WorldContextObject)
                        {
                                if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
                                {
                                        return World;
                                }
                        }

                        if (UGameViewportClient* Viewport = GEngine->GameViewport)
                        {
                                if (UWorld* ViewportWorld = Viewport->GetWorld())
                                {
                                        if (IsPreferredWorldType(ViewportWorld->WorldType))
                                        {
                                                return ViewportWorld;
                                        }
                                }
                        }

                        UWorld* FallbackWorld = nullptr;

                        const TIndirectArray<FWorldContext>& Contexts = GEngine->GetWorldContexts();
                        for (const FWorldContext& Context : Contexts)
                        {
                                if (UWorld* ContextWorld = Context.World())
                                {
                                        if (IsPreferredWorldType(Context.WorldType))
                                        {
                                                return ContextWorld;
                                        }

                                        if (!FallbackWorld)
                                        {
                                                FallbackWorld = ContextWorld;
                                        }
                                }
                        }

                        if (FallbackWorld)
                        {
                                return FallbackWorld;
                        }
                }

                if (GWorld && IsPreferredWorldType(GWorld->WorldType))
                {
                        return GWorld;
                }

                return GWorld;
        }
}
