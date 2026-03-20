#pragma once
#include "CoreMinimal.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Types/OnlineSessionData.h"


namespace NexusOnline
{
	//───────────────────────────────────────────────
	// Constants (Optimized)
	//───────────────────────────────────────────────
	inline const FName SESSION_KEY_PROJECT_ID_INT = FName("ProjectID");
	inline const int32 PROJECT_ID_VALUE_INT = 888888;

	//───────────────────────────────────────────────
	// ID Generator
	//───────────────────────────────────────────────
	inline FString GenerateRandomSessionId(int32 Length)
	{
		static const FString Charset = TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
		
		FString Result;
		Result.Reserve(Length);
		for (int32 i = 0; i < Length; ++i)
		{
			int32 Index = FMath::RandRange(0, Charset.Len() - 1);
			Result.AppendChar(Charset[Index]);
		}
		
		return Result;
	}
	
	//───────────────────────────────────────────────
	// ENUM ↔ NAME Conversion
	//───────────────────────────────────────────────
	inline FName SessionTypeToName(ENexusSessionType Type)
	{
		switch (Type)
		{
		case ENexusSessionType::GameSession:
			return NAME_GameSession;
			
		case ENexusSessionType::PartySession:
			return NAME_PartySession;
			
		case ENexusSessionType::SpectatorSession:
			return FName(TEXT("SpectatorSession"));
			
		case ENexusSessionType::CustomSession:
			return FName(TEXT("CustomSession"));
			
		default: 
				return NAME_GameSession;
		}
	}

	inline ENexusSessionType NameToSessionType(const FName& Name)
	{
		if (Name == NAME_PartySession)
			return ENexusSessionType::PartySession;
		
		if (Name == FName(TEXT("SpectatorSession")))
			return ENexusSessionType::SpectatorSession;
		
		if (Name == FName(TEXT("CustomSession")))
			return ENexusSessionType::CustomSession;
		
		return ENexusSessionType::GameSession;
	}

	//───────────────────────────────────────────────
	// Subsystem Access Helpers
	//───────────────────────────────────────────────
	inline IOnlineSubsystem* GetSubsystem(UObject* WorldContextObject)
	{
		if (!WorldContextObject)
			return nullptr;
		
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
		return World ? Online::GetSubsystem(World) : nullptr;
	}

	inline bool IsSubsystemAvailable(UObject* WorldContextObject)
	{
		return (GetSubsystem(WorldContextObject) != nullptr);
	}

	//───────────────────────────────────────────────
	// Interface Accessors
	//───────────────────────────────────────────────
	inline IOnlineSessionPtr GetSessionInterface(UObject* WorldContextObject)
	{
		if (IOnlineSubsystem* Subsystem = GetSubsystem(WorldContextObject))
			return Subsystem->GetSessionInterface();
		
		return nullptr;
	}

	inline IOnlineIdentityPtr GetIdentityInterface(UObject* WorldContextObject)
	{
		if (IOnlineSubsystem* Subsystem = GetSubsystem(WorldContextObject))
			return Subsystem->GetIdentityInterface();
		
		return nullptr;
	}

	inline IOnlineFriendsPtr GetFriendsInterface(UObject* WorldContextObject)
	{
		if (IOnlineSubsystem* Subsystem = GetSubsystem(WorldContextObject))
			return Subsystem->GetFriendsInterface();
		
		return nullptr;
	}

	inline IOnlineExternalUIPtr GetExternalUIInterface(UObject* WorldContextObject)
	{
		if (IOnlineSubsystem* Subsystem = GetSubsystem(WorldContextObject))
			return Subsystem->GetExternalUIInterface();
		
		return nullptr;
	}

	inline IOnlinePresencePtr GetPresenceInterface(UObject* WorldContextObject)
	{
		if (IOnlineSubsystem* Subsystem = GetSubsystem(WorldContextObject))
			return Subsystem->GetPresenceInterface();
		
		return nullptr;
	}
}