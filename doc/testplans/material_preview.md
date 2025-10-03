# Material Preview

## Overview

Material preview is a UI feature which displays a lit spherical preview of a PBR material. It can be found in the following UIs:

- The material picker swatch
    - In the build floater, in the Texture tab, when applying a PBR material
    - (If the feature is enabled) In the Region/Estate floater, in the Terrain tab, when applying PBR materials to terrain
- In the floater to select a material from inventory, which can be opened by clicking the material picker swatch

## Known Issues

These are known issues that the current implementation of this feature does not address:

- The material preview in the build floater is a preview of the base material ID only, and ignores other properties on the prim face like material overrides (https://github.com/secondlife/viewer/issues/865)
- Alpha mask previews as alpha blend (https://github.com/secondlife/viewer/issues/866)
- Double-sided previews as single-sided (https://github.com/secondlife/viewer/issues/867)
- Material preview inherits some of its lighting from the current environment, and reflections from the default reflection probe (https://github.com/secondlife/viewer/issues/868)

## General Regression Testing

- Check that the material preview swatch looks OK with different materials selected
- Check that the material preview swatch runs reasonably well on different systems, especially when the select material from inventory floater is also open
    - In particular: AMD, MacOS, minimum spec machines
- Watch out for regressions in rendering caused by opening a floater with a material preview swatch

## Bug Fixes

### Disappearing Objects Fix Test

This test is recommended for verifying that https://github.com/secondlife/viewer-issues/issues/72 is fixed.

#### Symptoms

When the bug occurs, one or more of following types of objects could randomly disappear in the world, permanently until relog:

- Objects
- Water level in current region
- Adjacent region/void water

Note: Disappearing objects in reflections have a different root cause and are not covered by the fix.

#### Bug Reproduction Steps

Verify the disappearing objects bug does not reproduce, given the following steps:

- Runtime prerequisites: Material preview swatch may not be available in your viewer or region if this feature is still behind a feature flag. It is safe to enable this feature manually by setting, "UIPreviewMaterial" to True in the advanced settings. The setting will persist for the current session.
- Region prerequisites: Unknown, but a region with lots of objects in it seems to increase repro rate. The following locations have been known to easily reproduce the bug, as of 2024-02-16:
    - http://maps.secondlife.com/secondlife/LindenWorld%20B/161/75/47
    - [secondlife://Aditi/secondlife/Rumpus%20Room%202048/128/128/24](secondlife://Aditi/secondlife/Rumpus%20Room%202048/128/128/24)
- Right click an object and select, "Edit item"
- Go to texture tab, select PBR Metallic Roughness from dropdown, and click the button to select material from inventory
- Ensure "Apply now" is checked in the inventory selection floater
- Alternate between different materials from the inventory selection floater
- Look around the world and check for permanently disappeared objects.

### Dynamic Exposure Influence Fix Test

This test is recommended for verifying that https://github.com/secondlife/viewer-issues/issues/72 is fixed.

#### Symptoms

Dynamic exposure in the world could be influenced by the material preview being displayed. If a material preview was being generated in a given frame, then, depending on the current environment, the user would observe an unpleasant flashing effect in the environment:

- The world view could suddenly get darker and then fade back to normal exposure levels
- The world view could suddenly get brighter and then fade back to normal exposure levels

#### Bug Reproduction Steps

Verify the dynamic exposure influence bug does not reproduce. Test using a few environment presets such as Default Midday, Sunset, and Midnight.

- Right click an object and select, "Edit item"
- Go to texture tab, select PBR Metallic Roughness from dropdown, and click the button to select material from inventory
- Alternate between different materials from the inventory selection floater

#### Regression Testing

Dynamic exposure in the world should continue to work correctly. In particular:

- Exposure should fade gradually from high exposure to low exposure and back as needed
- Exposure should decrease in brighter environments
- Exposure should increase in darker environments
