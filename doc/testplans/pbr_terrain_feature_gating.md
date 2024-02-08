# PBR Terrain Feature Gating

PBR terrain should have lower detail on lower graphics settings. PBR terrain will also not show emissive textures on some machines (like Macs) which do not support more than 16 textures.

## Triplanar Mapping

Triplanar mapping improves the texture repeats on the sides of terrain slopes.

Availability of Triplanar mapping:

- Medium-High and below: No triplanar mapping
- High and above: Triplanar mapping

## PBR Textures

At the highest graphics support level, PBR terrain supports all PBR textures.

Availability of PBR textures varies by machine and graphics setting:

- Low: Base color only (looks similar to texture terrain)
- Medium-Low, and machines that do not support greater than 16 textures such as Macs: All PBR textures enabled except emissive textures.
- Medium: All PBR textures enabled
