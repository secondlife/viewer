# Terrain Loading

## Behavior overview

- Texture terrain should load if applied
- PBR terrain should load if applied, even if the feature flag is disabled
- Related subsystem: A change to the PBR terrain loading system may affect the texture terrain loading system and vice-versa
- Related subsystem: Minimap should load if terrain loads
    - They may not finish loading at the same time

## Implementation details

This section is provided mainly for clarification of how the terrain loading system works.

The simulator sends 4 terrain composition UUIDs to the viewer for the region. The viewer does not know ahead-of-time if the terrain composition uses textures or materials. Therefore, to expedite terrain loading, the viewer makes up to 8 "top-level" asset requests simultaneously:

- Up to 4 texture asset requests, one for each UUID
- Up to 4 material asset requests, one for each UUID

It is therefore expected that half of these asset lookups will fail.

The viewer inspects the load success of these top-level assets to make the binary decision of whether to render all 4 texture assets or all 4 material assets. This determines the choice of composition for terrain both in-world and on the minimap.

The minimap also attempts to wait for textures to partially load before it can render a tile for a given region:

- When rendering texture terrain, the minimap attempts to wait for top-level texture assets to partially load
- When rendering PBR material terrain, the minimap attempts to wait for any base color/emissive textures in the materials to partially load, if they are present

We don't make guarantees that the minimap tile will render for the region if any aforementioned required textures/materials fail to sufficiently load. However, the minimap may make a best-effort attempt to render the region by ignoring or replacing data.
