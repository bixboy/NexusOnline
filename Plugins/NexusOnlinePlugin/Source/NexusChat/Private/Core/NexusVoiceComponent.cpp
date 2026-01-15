#include "Core/NexusVoiceComponent.h"
#include "Core/NexusChatComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Types/NexusVoiceConfig.h"
#include "Core/NexusChatSubsystem.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <sapi.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

UNexusVoiceComponent::UNexusVoiceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNexusVoiceComponent::BeginPlay()
{
	Super::BeginPlay();

#if PLATFORM_WINDOWS
	ISpVoice* pVoice = nullptr;
	if (SUCCEEDED(CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&pVoice)))
	{
		VoiceInterface = pVoice;
	}
#endif

	if (AActor* Owner = GetOwner())
	{
		ChatComponent = Owner->FindComponentByClass<UNexusChatComponent>();
		if (ChatComponent)
		{
			ChatComponent->OnMessageReceived.AddDynamic(this, &UNexusVoiceComponent::OnChatReceived);
		}
	}
}

void UNexusVoiceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if PLATFORM_WINDOWS
	if (VoiceInterface)
	{
		static_cast<ISpVoice*>(VoiceInterface)->Release();
		VoiceInterface = nullptr;
	}
#endif

	if (ChatComponent)
	{
		ChatComponent->OnMessageReceived.RemoveDynamic(this, &UNexusVoiceComponent::OnChatReceived);
	}

	Super::EndPlay(EndPlayReason);
}

void UNexusVoiceComponent::OnChatReceived(const FNexusChatMessage& Message)
{
	if (!VoiceConfig)
		return;

	// Respect Chat Filters (Active Tab)
	if (UWorld* World = GetWorld())
	{
		if (UNexusChatSubsystem* Subsystem = World->GetSubsystem<UNexusChatSubsystem>())
		{
			if (!Subsystem->IsMessagePassesFilter(Message))
			{
				return;
			}
		}
	}

	if (!VoiceConfig->AllowedChannels.IsEmpty() && !VoiceConfig->AllowedChannels.Contains(Message.Channel))
		return;
	
	if (!VoiceConfig->bHearOwnVoice)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
		{
			if (PC->PlayerState && PC->PlayerState->GetPlayerName() == Message.SenderName)
			{
				return;
			}
		}
	}

	int32 FinalPitch = VoiceConfig->BasePitch;
	int32 FinalRate = VoiceConfig->BaseRate;

	if (VoiceConfig->bRandomizeVoicePerPlayer && !Message.SenderName.IsEmpty())
	{
		uint32 NameHash = GetTypeHash(Message.SenderName);
		
		int32 PitchVariation = (NameHash % 10) - 5; 
		FinalPitch = FMath::Clamp(VoiceConfig->BasePitch + PitchVariation, -10, 10);

		int32 RateVariation = (NameHash % 4) - 2;
		FinalRate = FMath::Clamp(VoiceConfig->BaseRate + RateVariation, -10, 10);
	}

	Speak(Message.MessageContent, FinalRate, FinalPitch);
}

void UNexusVoiceComponent::Speak(const FString& Text, int32 Rate, int32 Pitch)
{
#if PLATFORM_WINDOWS
	if (!VoiceInterface)
		return;

	ISpVoice* pVoice = static_cast<ISpVoice*>(VoiceInterface);
	
	int32 Vol = VoiceConfig ? VoiceConfig->Volume : 100;
	pVoice->SetVolume(static_cast<USHORT>(Vol));

	FString XmlText = FString::Printf(TEXT("<pitch absmiddle=\"%d\"><rate absspeed=\"%d\">%s</rate></pitch>"), Pitch, Rate, *Text);
	pVoice->Speak(*XmlText, SPF_ASYNC | SPF_IS_XML | SPF_PURGEBEFORESPEAK, NULL);
#endif
}