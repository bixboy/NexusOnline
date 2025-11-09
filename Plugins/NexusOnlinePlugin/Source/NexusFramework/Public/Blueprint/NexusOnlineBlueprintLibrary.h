#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Async/AsyncTask_CreateSession.h"
#include "Async/AsyncTask_DestroySession.h"
#include "Async/AsyncTask_FindSessions.h"
#include "Async/AsyncTask_JoinSession.h"
#include "NexusOnlineBlueprintLibrary.generated.h"


UCLASS()
class NEXUSFRAMEWORK_API UNexusOnlineBlueprintLibrary : public UBlueprintFunctionLibrary
{
        GENERATED_BODY()

public:

        /**
         * Récupère dynamiquement le nombre de joueurs inscrits dans une session donnée.
         * @return true si la session est trouvée et les informations renseignées.
         */
        UFUNCTION(BlueprintCallable, Category="Nexus|Online|Session", meta=(WorldContext="WorldContextObject"))
        static bool GetSessionPlayerCounts(UObject* WorldContextObject, ENexusSessionType SessionType, int32& CurrentPlayers, int32& MaxPlayers);
};

