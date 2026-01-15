#include "UI/NexusChatChannelEntry.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "UI/NexusChatWindow.h"
#include "Kismet/GameplayStatics.h"

void UNexusChatChannelEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	MyData = Cast<UNexusChatChannelData>(ListItemObject);

	if (MyData && ChannelNameText)
	{
		ChannelNameText->SetText(FText::FromName(MyData->ChannelName));
        
        // Optional styling logic based on private/public could go here
        if (BackgroundBorder)
        {
            // Reset or set color
        }
	}
}

FReply UNexusChatChannelEntry::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (MyData)
	{
		// Find the parent window to call SelectTab
		// Ideally, we would use a delegate, but traversing up is quick for simpler setups
		// Or utilize the list owner. For now, let's try to find the NexusChatWindow in the outer chain 
		// or rely on a delegate pattern if we want to be stricter.
		
		// Let's use a simpler approach: Broadcast an event or find the ChatWindow.
        // Since this is a plugin, we can try to get the ChatWindow from the PlayerController or similar if it's unique.
        // BUT, the Entry is inside a List, inside a Widget, inside the Window.
        
        // Better approach: User clicks -> this widget handles it.
        // We can cast the Outer to find the Window, but that's brittle if hierarchy changes.
        // Actually, the ListView owns these. 
        
        // Let's use a delegate on the Data Object or a globally accessible event? 
        // No, standard UI pattern:
        // The ListView verifies selection.
        // Actually, ListView has "OnItemUsed" or "OnSelectionChanged".
        // SO we might not even need OnMouseButtonDown if we use the List's selection mechanics!
        // However, the user asked for "Clicking an item".
        // Let's stick to standard ListView selection behavior if possible.
        // If we want manual click:
        
		if (APlayerController* PC = GetOwningPlayer())
		{
            // Hacky but works for now: Find the widget instance in the viewport? No.
            // Let's iterate pure UMG hierarchy parents.
            UWidget* Parent = GetParent();
            while (Parent)
            {
                if (UNexusChatWindow* Window = Cast<UNexusChatWindow>(Parent))
                {
                    Window->SelectTab(MyData->ChannelName, !MyData->bIsPrivate);
                    // Also close the list
                    // Window->ToggleChannelList(false); // We need to add this method or handle it
                    return FReply::Handled();
                }
                 // Special handling because ListView entries are in a different validity tree sometimes
                 // But usually GetParent works for standard tree.
                 Parent = Parent->GetParent();
            }
            
            // If tree traversal fails (common with ListViews), fallback to finding via Controller component?
            // Or easier: The ChannelList widget catches the "OnItemSelectionChanged" from its ListView.
            // YES. Let's do that. It's much cleaner. The Entry just displays data.
            // So this OnMouseButtonDown isn't strictly needed if we use ListView properly.
		}
	}
    
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
