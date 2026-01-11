#pragma once

#include "CoreMinimal.h"
#include "NexusChatTypes.generated.h"

UENUM(BlueprintType)
enum class ENexusChatChannel : uint8
{
	Global		UMETA(DisplayName = "Global"),
	Team		UMETA(DisplayName = "Team"),
	Party		UMETA(DisplayName = "Party"),
	Whisper		UMETA(DisplayName = "Whisper"),
	System		UMETA(DisplayName = "System"),
	GameLog		UMETA(DisplayName = "GameLog"),
	Custom      UMETA(DisplayName = "Custom")
};

USTRUCT(BlueprintType)
struct NEXUSCHAT_API FNexusChatMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FString SenderName;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FString MessageContent;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FDateTime Timestamp;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	ENexusChatChannel Channel;
	
	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FName ChannelName;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	int32 SenderTeamId;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	int32 SenderPartyId;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FString TargetName;

	FNexusChatMessage()
		: Timestamp(0)
		, Channel(ENexusChatChannel::Global)
		, ChannelName(NAME_None)
		, SenderTeamId(-1)
		, SenderPartyId(-1)
	{}
	
	static FNexusChatMessage MakeSystem(const FString& Content)
	{
		FNexusChatMessage Msg;
		Msg.Channel = ENexusChatChannel::System;
		Msg.MessageContent = Content;
		Msg.SenderName = TEXT("System");
		Msg.Timestamp = FDateTime::Now();
		return Msg;
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << SenderName;
		Ar << MessageContent;
		Ar << Timestamp;
		
		uint8 Ch = static_cast<uint8>(Channel);
		Ar << Ch;
		if (Ar.IsLoading())
		{
			Channel = static_cast<ENexusChatChannel>(Ch);
		}
		
		Ar << ChannelName;

		Ar << SenderTeamId;
		Ar << SenderPartyId;
		Ar << TargetName;

		bOutSuccess = true;
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FNexusChatMessage> : TStructOpsTypeTraitsBase2<FNexusChatMessage>
{
	enum
	{
		WithNetSerializer = true
	};
};