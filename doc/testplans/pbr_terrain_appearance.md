# PBR Terrain Appearance

## Tiling

The southwest corner of a region with PBR materials should exactly match up with the bottom left corner of the material texture(s).

If two adjacent regions have the same PBR terrain settings, then:

- There should not be seams between the two regions at their shared border
- The ground should not suddenly slide beneath the avatar when moving between regions (except due to movement of the avatar, which is not covered by this test plan)

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
