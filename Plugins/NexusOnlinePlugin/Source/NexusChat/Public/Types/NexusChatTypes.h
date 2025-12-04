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
	GameLog		UMETA(DisplayName = "GameLog")
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
	int32 SenderTeamId;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FString TargetName;

	FNexusChatMessage()
		: Timestamp(0)
		, Channel(ENexusChatChannel::Global)
		, SenderTeamId(-1)
	{}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << SenderName;
		Ar << MessageContent;
		Ar << Timestamp;
		Ar << Channel;
		
		if (Channel == ENexusChatChannel::Team)
		{
			Ar << SenderTeamId;
		}

		if (Channel == ENexusChatChannel::Whisper)
		{
			Ar << TargetName;
		}

		bOutSuccess = true;
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FNexusChatMessage> : public TStructOpsTypeTraitsBase2<FNexusChatMessage>
{
	enum
	{
		WithNetSerializer = true
	};
};
