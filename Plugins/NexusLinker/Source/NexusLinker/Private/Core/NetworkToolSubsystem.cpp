#include "Core/NetworkToolSubsystem.h"
#include "Core/NetworkToolSettings.h"
#include "Editor.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Framework/Application/SlateApplication.h" // Required for FDisplayMetrics

void UNetworkToolSubsystem::LaunchLocalCluster(int32 ClientCount, bool bRunDedicatedServer)
{
    ULevelEditorPlaySettings* PlaySettings = GetMutableDefault<ULevelEditorPlaySettings>();
    if (!PlaySettings) return;

    // 1. Grid Calculation
    int32 DesktopWidth = 1920;
    int32 DesktopHeight = 1080;
    
    // Try to get actual screen size
    if (FSlateApplication::Is
Initialized()) // Check if SlateApplication is initialized before trying to get display metrics
    {
        FDisplayMetrics Metrics;
        FSlateApplication::Get().GetDisplayMetrics(Metrics);
        DesktopWidth = Metrics.PrimaryDisplayWorkAreaRect.Right - Metrics.PrimaryDisplayWorkAreaRect.Left;
        DesktopHeight = Metrics.PrimaryDisplayWorkAreaRect.Bottom - Metrics.PrimaryDisplayWorkAreaRect.Top;
    }

    int32 Rows = 1;
    int32 Cols = 1;

    // determine grid (e.g. 2 -> 2x1, 3 -> 2x2, 4 -> 2x2)
    if (ClientCount > 1) { Cols = 2; Rows = (ClientCount + 1) / 2; }
    if (ClientCount > 4) { Cols = 3; Rows = (ClientCount + 2) / 3; }

    int32 WinW = DesktopWidth / Cols;
    int32 WinH = DesktopHeight / Rows;

    // Apply Settings
    PlaySettings->SetPlayWindowWidth(WinW);
    PlaySettings->SetPlayWindowHeight(WinH);
    PlaySettings->SetPlayWindowPosition(FIntPoint(50, 50)); // Default Start Pos

    // 2. Configuration du mode réseau
    if (bRunDedicatedServer)
    {
        // Serveur Dédié + Clients
        PlaySettings->SetPlayNetMode(EPlayNetMode::PIE_Client);
        PlaySettings->SetRunUnderOneProcess(false);
        PlaySettings->SetPlayNumberOfClients(ClientCount + 1);

        PlaySettings->bLaunchSeparateServer = true; 
    }
    else
    {
        // Listen Server (Host) + Clients
        PlaySettings->SetPlayNetMode(EPlayNetMode::PIE_ListenServer);
        // Note: RunUnderOneProcess = true often forces windows to stack or tab
        // To visualize grid, we might prefer separate processes, but user asked for logic update only.
        // We keep existing logic for process mode.
        PlaySettings->SetRunUnderOneProcess(true);
        PlaySettings->SetPlayNumberOfClients(ClientCount);
    }

    // 3. Sauvegarde temporaire et Lancement
    PlaySettings->PostEditChange();

    FRequestPlaySessionParams SessionParams;
    GEditor->RequestPlaySession(SessionParams);
    
    UE_LOG(LogTemp, Log, TEXT("[NetworkTool] Local Cluster Started with %d clients (Grid %dx%d)."), ClientCount, Cols, Rows);
}

void UNetworkToolSubsystem::JoinServerProfile(const FServerProfile& Profile)
{
    if (Profile.IPAddress.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[NetworkTool] IP address is empty!"));
        return;
    }

    ULevelEditorPlaySettings* PlaySettings = GetMutableDefault<ULevelEditorPlaySettings>();
    if (!PlaySettings)
        return;

    // 1. On se met en mode Client seul
    PlaySettings->SetPlayNetMode(EPlayNetMode::PIE_Client);
    PlaySettings->SetPlayNumberOfClients(1);
    PlaySettings->bLaunchSeparateServer = false;
    PlaySettings->PostEditChange();

    // 2. On s'abonne au démarrage du PIE pour injecter la commande "open IP"
    FString IPToJoin = Profile.IPAddress;
    int32 PortToJoin = Profile.Port;
    FString MapName = Profile.MapName;
    FString ExtraArgs = Profile.ExtraArgs;

    FEditorDelegates::PostPIEStarted.AddWeakLambda(this, [IPToJoin, PortToJoin, MapName, ExtraArgs](bool bIsSimulating)
    {
        if (!bIsSimulating && GEditor->GetPIEWorldContext())
        {
            FString Command = FString::Printf(TEXT("open %s:%d"), *IPToJoin, PortToJoin);
            
            // Append MapName (User logic: append args)
            if (!MapName.IsEmpty())
            {
                 // If it looks like a path, careful not to break IP.
                 // Assuming IP:Port/MapName format or similar if intended?
                 // Spec says "Execute... append arguments".
                 // We append it.
                 Command += MapName;
            }

            // Append ExtraArgs
            if (!ExtraArgs.IsEmpty())
            {
                 // Check if we need separator
                 if (!Command.Contains(TEXT("?")) && !ExtraArgs.StartsWith(TEXT("?")))
                 {
                     Command += TEXT("?");
                 }
                 Command += ExtraArgs;
            }

            GEngine->Exec(GEditor->GetPIEWorldContext()->World(), *Command);
            
            UE_LOG(LogTemp, Log, TEXT("[NetworkTool] Auto-Connecting... Cmd: %s"), *Command);
        }
    });

    // 3. Lancer la session
    FRequestPlaySessionParams SessionParams;
    GEditor->RequestPlaySession(SessionParams);
}

void UNetworkToolSubsystem::SendRemoteCommand(const FServerProfile& Profile, FString CommandType)
{
    FHttpModule* Http = &FHttpModule::Get();
    if (!Http)
        return;

    TSharedRef<IHttpRequest> Request = Http->CreateRequest();

    // Choix de l'URL selon la commande
    FString TargetUrl = (CommandType == "START") ? Profile.ApiUrl_Start : Profile.ApiUrl_Stop;
    
    if (TargetUrl.IsEmpty())
    {
        OnServerStatusLog.Broadcast(FString::Printf(TEXT("Error: No URL configured for %s"), *CommandType));
        return;
    }

    Request->SetURL(TargetUrl);
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    
    if (!Profile.AuthToken.IsEmpty())
    {
        Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *Profile.AuthToken));
    }

    Request->OnProcessRequestComplete().BindUObject(this, &UNetworkToolSubsystem::OnRemoteCommandComplete);
    Request->ProcessRequest();
    
    OnServerStatusLog.Broadcast(FString::Printf(TEXT("Sending %s command to %s..."), *CommandType, *Profile.ProfileName));
}

void UNetworkToolSubsystem::OnRemoteCommandComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        FString ResponseStr = Response->GetContentAsString();
        int32 Code = Response->GetResponseCode();

        if (Code >= 200 && Code < 300)
        {
            OnServerStatusLog.Broadcast(FString::Printf(TEXT("Success (%d): %s"), Code, *ResponseStr));
        }
        else
        {
            OnServerStatusLog.Broadcast(FString::Printf(TEXT("Failed (%d): %s"), Code, *ResponseStr));
        }
    }
    else
    {
        OnServerStatusLog.Broadcast(TEXT("Connection Error: Check URL or Internet."));
    }
}

void UNetworkToolSubsystem::SetNetworkEmulation(const FNetworkEmulationProfile& Profile)
{
    IConsoleManager& IConsole = IConsoleManager::Get();

    // PktLag = Latence (Ping)
    if (IConsoleVariable* CVar = IConsole.FindConsoleVariable(TEXT("Net.PktLag")))
    {
        CVar->Set(Profile.PktLag, ECVF_SetByCode);
    }
    
    // PktLoss = Perte de paquets
    if (IConsoleVariable* CVar = IConsole.FindConsoleVariable(TEXT("Net.PktLoss")))
    {
        CVar->Set(Profile.PktLoss, ECVF_SetByCode);
    }

    UE_LOG(LogTemp, Warning, TEXT("[NetworkTool] Applied Lag: %dms, Loss: %d%%"), Profile.PktLag, Profile.PktLoss);
}