# Set UI Size to Default - Feature Specification

## Summary
Add a "Set UI Size to Default" option to the Help menu on the login screen, allowing users to easily reset their UI scale.

## Rationale
Users who accidentally set extreme UI scaling cannot easily navigate to preferences to reset it. A help menu item on the login screen provides a recovery path.

## Implementation

### Menu Addition
```
Help Menu:
  ├── About...
  ├── Set UI Size to Default  ← NEW
  └── Quit
```

### Code Changes

#### 1. Menu Definition (`indra/newview/lllogininstance.cpp`)
```cpp
// Add to help menu on login screen
help_menu->append(new LLMenuItemCallGL(
    "Set UI Size to Default",
    [](void*) { setDefaultUISize(); },
    NULL,  // enabled callback
    NULL,  // userdata
    "Set_UI_Size_To_Default"
));
```

#### 2. Reset Function
```cpp
void setDefaultUISize()
{
    // Reset UI scale to 1.0
    gSavedSettings.setF32("UIScaleFactor", 1.0f);
    
    // Reset font size
    gSavedSettings.setF32("FontSize", 1.0f);
    
    // Apply changes
    gViewerWindow->reshapeWindow();
    
    // Show confirmation
    LLNotifications::instance().add("UISizeReset");
}
```

#### 3. Strings (`english_strings.xml`)
```xml
<string name="Set_UI_Size_To_Default">Set UI Size to Default</string>
<string name="UISizeReset">UI size has been reset to default. Changes will take effect immediately.</string>
```

### Testing
1. Set UI scale to maximum
2. Verify login screen is still navigable
3. Click Help → Set UI Size to Default
4. Verify UI resets to 1.0
5. Verify no restart required
