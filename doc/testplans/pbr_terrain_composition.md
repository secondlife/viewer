# PBR Terrain Composition

## Feature Availability

PBR Terrain is visible for all viewers with the PBR Terrain feature, regardless of if the feature flag is enabled.

There is only one set of four asset IDs applied to the terrain. In other words, unlike PBR materials on prims, there is no fallback texture set for viewers that do not support PBR terrain. Viewers without support will view terrain as blank (solid grey or white).

## Editing Terrain Composition

All tests in this section assume the PBR terrain feature flag is enabled, and that the user has appropriate permissions to modify the terrain textures.

### Feature Availability

These features are related to UI, where the Region/Estate floater is opened to the terrain Tab.

#### Feature: PBR Terrain

On the client, the advanced setting `RenderTerrainPBREnabled` is the PBR terrain feature flag.

The PBR terrain feature flag should be set automatically when logging in/teleporting to a new region.

- The flag should be enabled on regions where the PBR terrain feature is enabled
- Otherwise the flag should be disabled

When the PBR terrain feature flag is disabled:

- The "PBR Metallic Roughness" checkbox should not be visible
- The user should not be able to apply PBR terrain to the region, only textures.

When the PBR terrain feature flag is enabled:

- The "PBR Metallic Roughness" checkbox should be visible
- The user should be able to apply PBR terrain or textures to the region, depending on if the "PBR Metallic Roughness" checkbox is checked.

#### Feature: PBR Terrain Texture Transforms

On the client, the advanced setting, `RenderTerrainPBRTransformsEnabled` is the PBR terrain texture transform flag. Generally, this feature should not be expected to work correctly unless the PBR terrain feature is also enabled.

The PBR terrain texture transform flag should be set automatically when logging in/teleporting to a new region.

- The flag should be enabled on regions where the PBR terrain texture transform feature is enabled
- Otherwise the flag should be disabled

When the PBR terrain texture transform feature is enabled, the UI of the Terrain tab should be overhauled. Availability of features depends on the type of terrain.

**Known issue:** The Region/Estate floater may have to be closed/reopened a second time in order for the UI overhaul to take effect, after teleporting between regions that do and do not have the feature flag set.

When "PBR Metallic Roughness" is checked:

- There should be a way for the user to change the texture transforms for the terrain in the current region
- For each of the four swatches, the user can change the scale, offset, and rotation of that swatch. Nonuniform scale is allowed

When "PBR Metallic Roughness" is unchecked, the controls for texture transforms should be hidden.

### Current Composition Type

When the Region/Estate floater is opened to the terrain Tab, the current terrain should be shown in the four swatches, and the "PBR Metallic Roughness" checkbox should be checked or unchecked accordingly.

- If it is texture terrain, the "PBR Metallic Roughness" checkbox should be unchecked, and the floater should display the four textures applied to the terrain.
- If it is material terrain, the "PBR Metallic Roughness" checkbox should be checked, and the floater should display the four materials applied to the terrain.

In addition, where possible, textual labels and descriptions in the tab should make sense given the current value of the "PBR Metallic Roughness" checkbox. If the checkbox is unchecked, the labels should refer to textures. If the checkbox is checked, the labels should refer to materials.

### Toggling Composition Type

When toggling the "PBR Metallic Roughness" checkbox to the opposite value, which does not correspond to the current terrain type, one of the following sets of four terrain swatches will be displayed:

- The default textures/materials
    - For textures, this is the default terrain texture set
    - For materials, this is all blank materials, but this is subject to change
- The previously applied texture/material set
    - History is available on a best-effort basis only. In particular, the history does not persist on viewer restart.

When toggling back the "PBR Metallic Roughness" checkbox to the original value, assuming nothing else has changed, then the current terrain should be shown in the four swatches again.

### Saving Composition

A user with appropriate permissions can change and save the textures or materials to the terrain. If the "PBR Metallic Roughness" checkbox is checked, the user applies materials, otherwise the user applies textures.

The user should not be allowed to set the texture or material swatches to null.

Saving may fail for the following reasons:

- A terrain or material texture is invalid
- A terrain texture is greater than the max texture upload resolution

If saving the terrain fails for any reason, the terrain should not be updated.

Unlike a viewer without PBR terrain support, the new viewer will no longer treat textures with alpha channels as invalid.

### Saving PBR Terrain Texture Transforms

If "PBR Metallic Roughness" checkbox is checked, a user with saving composition permissions should also be allowed to edit and save PBR texture transforms.

One texture transform may be set for each material swatch. Setting texture transforms for each individual texture on the material is not currently supported.

## Graphics Features

Texture terrain with transparency is not permitted to be applied in the viewer.

See [PBR Terrain Appearance](./pbr_terrain_appearance.md) for supported PBR terrain features.

## Minimap

The minimap should display the terrain with appropriate textures and colors.
