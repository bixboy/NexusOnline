#pragma once
#include "CoreMinimal.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Types/OnlineSessionData.h"

namespace NexusOnline
{
	/** Conversion d’un type de session (enum) vers son FName interne */
	static FORCEINLINE FName SessionTypeToName(ENexusSessionType Type)
	{
		switch (Type)
		{
		case ENexusSessionType::PartySession:
			return FName("PartySession");
			
		case ENexusSessionType::SpectatorSession:
			return FName("SpectatorSession");
			
		case ENexusSessionType::CustomSession:
			return FName("CustomSession");
			
		default:
			return FName("GameSession");
		}
	}

	/** Conversion inverse FName → enum */
	static FORCEINLINE ENexusSessionType NameToSessionType(const FName& Name)
	{
		if (Name == "PartySession")
			return ENexusSessionType::PartySession;
		
		if (Name == "SpectatorSession")
			return ENexusSessionType::SpectatorSession;
		
		if (Name == "CustomSession")
			return ENexusSessionType::CustomSession;
		
		return ENexusSessionType::GameSession;
	}

	/** Check si le subsystem est disponible */
	static FORCEINLINE bool IsSubsystemAvailable(UWorld* World)
	{
		return (Online::GetSubsystem(World) != nullptr);
	}

	/** Retourne l'interface session */
	static FORCEINLINE IOnlineSessionPtr GetSessionInterface(UWorld* World)
	{
		if (IOnlineSubsystem* Subsystem = Online::GetSubsystem(World))
			return Subsystem->GetSessionInterface();
		
		return nullptr;
	}
}
