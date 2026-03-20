# NexusOnline Plugin

A modular Unreal Engine 5 plugin for online session management, built for multiplayer games.

## Modules

| Module | Description |
|---|---|
| **NexusFramework** | Core session management, async tasks, filtering, banning, migration |
| **NexusSteam** | Steam-specific utilities (friends, invites, presence, overlays) — optional |
| **NexusChat** | In-game chat system |
| **NexusOnlinePlugin** | Legacy wrapper module |

## Quick Start

### 1. Create a Session Config

Create a `UNexusSessionConfig` DataAsset to define your session settings (map, max players, session type, custom key-values).

### 2. Create / Find / Join Sessions

Use the Blueprint async nodes:
- **Create Session** / **Create Session From Config** — host a session
- **Find Sessions** — search with filters and sorting rules
- **Join Session** — connect to a found session
- **Destroy Session** — tear down the current session

### 3. Filter & Sort

- Subclass `USessionFilterRule` to create custom server-browser filters
- Subclass `USessionSortRule` for custom sorting
- Combine them in `USessionFilterPreset` DataAssets

### 4. Ban Management

`UNexusBanSubsystem` (GameInstanceSubsystem) — accessible from any GameMode:
```cpp
if (UNexusBanSubsystem* Bans = GetGameInstance()->GetSubsystem<UNexusBanSubsystem>())
{
    Bans->BanPlayer(PlayerId, FText::FromString("Cheating"));
    Bans->TempBanPlayer(PlayerId, 24.0f, FText::FromString("Toxicity"));
    Bans->UnbanPlayer(PlayerId);
}
```
- Cached in memory — no disk I/O per player login
- Auto-expires temporary bans
- Broadcasts `OnPlayerBanned` / `OnPlayerUnbanned` delegates

### 5. Custom GameMode Integration

You do **not** need to inherit from `ANexusGameMode`. Implement `INexusSessionHandler` on your own GameMode and use the static helpers:

```cpp
UCLASS()
class AMyGameMode : public AGameModeBase, public INexusSessionHandler
{
    GENERATED_BODY()

    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        INexusSessionHandler::InitializeNexusSession(this);
    }

    virtual void PostLogin(APlayerController* NewPlayer) override
    {
        Super::PostLogin(NewPlayer);
        INexusSessionHandler::RegisterPlayerWithSession(this, NewPlayer);
    }

    virtual void Logout(AController* Exiting) override
    {
        Super::Logout(Exiting);
        INexusSessionHandler::UnregisterPlayerFromSession(this, Exiting);
    }
};
```

### 6. Steam Utilities (Optional)

The `NexusSteam` module provides Blueprint-callable functions:
- `GetLocalSteamName` / `GetLocalSteamID`
- `ReadSteamFriends` / `GetCachedFriends`
- `InviteFriendToSession` / `ShowInviteOverlay` / `ShowProfileOverlay`
- `IsSteamActive` / `GetLocalPresence`

> **Note:** The `NexusSteam` module is optional. If you target EOS, Xbox, or PSN, simply don't include it — the core `NexusFramework` has no Steam dependency.

### 7. Session Migration

`UNexusMigrationSubsystem` handles host migration recovery. Configure retry count and delays in **Project Settings → Nexus Migration**.

## Architecture

```
NexusFramework (core, no platform dependencies)
├── Async Tasks (Create, Find, Join, Destroy, FindById, CreateFromConfig)
├── Filters & Sorting (SessionFilterRule, SessionSortRule, Presets)
├── Ban System (UNexusBanSubsystem)
├── Session Manager (AOnlineSessionManager — cached singleton)
├── Migration (UNexusMigrationSubsystem + UNexusMigrationConfig)
└── Interface (INexusSessionHandler)

NexusSteam (optional, Steam-only)
└── UNexusSteamUtils (friends, invites, presence, overlays)

NexusChat (independent)
└── Chat system
```
