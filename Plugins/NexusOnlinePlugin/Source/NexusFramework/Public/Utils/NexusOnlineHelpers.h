#pragma once
#include "CoreMinimal.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Types/OnlineSessionData.h"

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
}
