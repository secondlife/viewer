# PBR Terrain Appearance

## Tiling Without Texture Transforms

This section assumes the PBR terrain of the current region and adjacent regions have the default texture transforms.

The southwest corner of a region with PBR materials should exactly match up with the bottom left corner of the material texture(s).

If two adjacent regions have the same PBR terrain settings, then there should not be seams between the two regions at their shared border.

The ground should not suddenly slide beneath the avatar when moving between two PBR terrain regions (except due to movement of the avatar, which is not covered by this test plan)

## Feature Gating

PBR terrain should have lower detail on lower graphics settings. PBR terrain will also not show emissive textures on some machines (like Macs) which do not support more than 16 textures.

### Triplanar Mapping

Triplanar mapping improves the texture repeats on the sides of terrain slopes.

Availability of Triplanar mapping:

- Medium-High and below: No triplanar mapping
- High and above: Triplanar mapping

### PBR Textures

At the highest graphics support level, PBR terrain supports all PBR textures.

Availability of PBR textures varies by machine and graphics setting:

- Low: Base color only (looks similar to texture terrain)
- Medium-Low, and machines that do not support greater than 16 textures such as Macs: All PBR textures enabled except emissive textures.
- Medium: All PBR textures enabled

### PBR Alpha

PBR terrain does not support materials with alpha blend or double-sided. In addition, the viewer does not make any guarantees about what will render behind the terrain if alpha is used.

## PBR Terrain Texture Transforms

Like PBR materials on prims, PBR terrain repeats are based on the [KHR\_texture\_transform](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_texture_transform) spec, and thus should be expected to behave the same way. We currently suport offset, rotation, and scale from the spec. texCoord is not currently supported.

The southwest corner of a region, at z=0, is the UV origin for all texture coordinates of the whole region. Unless an offset is also applied, scale and rotation of the terrain texture transforms are relative to that point.

When an avatar faces north and looks down at flat ground, the textures of the materials should appear to face upright, unless a rotation is applied.

If triplanar mapping is enabled, and an avatar faces an axially-aligned wall, the textures of the materials should appear to face upright, unless a rotation is applied.

Textures of materials should not appear mirrored.

When triplanar mapping is enabled, rotations on the axially aligned walls should apply in the same direction as they would on flat ground.

## PBR Terrain Normal Textures

This section assumes terrain normal maps are enabled at the current graphics setting.

PBR terrain should have approximately correct lighting based on the normal texture:

- When on flat ground
- On cliffs, when triplanar mapping is enabled. Lighting will be somewhat less accurate when the cliff face is not axially aligned.
- If no Terrain Texture Transform is applied.
- If a Terrain Texture Transform is applied, especially for rotation or negative scale.
