# PBR Terrain Composition

## Feature Availability

PBR Terrain is visible for all viewers with the PBR Terrain feature, regardless of if the feature flag is enabled.

There is only one set of four asset IDs applied to the terrain. In other words, unlike PBR materials on prims, there is no fallback texture set for viewers that do not support PBR terrain. Viewers without support will view terrain as blank (solid grey or white).

## Editing Terrain Composition

All tests in this section assume the PBR terrain feature flag is enabled, and that the user has appropriate permissions to modify the terrain textures.

### Feature Availability

When the PBR terrain feature flag is disabled:

- The "PBR Metallic Roughness" checkbox should not be visible
- The user should not be able to apply PBR terrain to the region, only textures.

When the PBR terrain feature flag is enabled:

- The "PBR Metallic Roughness" checkbox should be visible
- The user should be able to apply PBR terrain or textures to the region, depending on if the "PBR Metallic Roughness" checkbox is checked.

### Current Composition Type

When the Region/Estate floater is opened to the terrain Tab, the current terrain should be shown in the four swatches, and the "PBR Metallic Roughness" checkbox should be checked or unchecked accordingly.

- If it is texture terrain, the "PBR Metallic Roughness" checkbox should be unchecked, and the floater should display the four textures applied to the terrain.
- If it is material terrain, the "PBR Metallic Roughness" checkbox should be checked, and the floater should display the four materials applied to the terrain.

### Toggling Composition Type

When toggling the "PBR Metallic Roughness" checkbox to the opposite value, which does not correspond to the current terrain type, one of the following sets of four terrain swatches will be displayed:

- The default textures/materials
    - For textures, this is the default terrain texture set
    - For materials, this is blank/null, but this is subject to change
- The previously applied texture/material set
    - History is available on a best-effort basis only. In particular, the history does not persist on viewer restart.

When toggling back the "PBR Metallic Roughness" checkbox to the original value, assuming nothing else has changed, then the current terrain should be shown in the four swatches again.

### Saving Composition

A user with appropriate permissions can change and save the textures or materials to the terrain. If the "PBR Metallic Roughness" checkbox is checked, the user applies materials, otherwise the user applies textures.

Saving may fail for the following reasons:

- A terrain or material texture is invalid or null
- A terrain texture is greater than 1024 resolution (Subject to change. See https://github.com/secondlife/viewer/issues/760)

Unlike a viewer without PBR terrain support, the new viewer will no longer treat textures with alpha channels as invalid.

## Graphics Features

Texture terrain with transparency will render as opaque. Parts of the texture that would be partially transparent will instead display as a mix of the color and black, depending on how transparent the texture is.

See [PBR Terrain Feature Gating](./pbr_terrain_feature_gating.md) for supported PBR terrain features.
