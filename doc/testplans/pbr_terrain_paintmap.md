# PBR Terrain Paintmap

## Introduction/Disclaimer

As of 2024-08-06, PBR terrain painting is **WIP**. Currently, there is only a client-side terrain paintmap, with no way to directly edit it. This document will explain how to informally explore this feature and compare it to the existing heightmap with noise. In the future, a testing document will be added for PBR terrain painting.

## Background

Historically, PBR terrain in a region has several parameters for controlling its composition. These are:

- The four materials
- The elevation of the terrain, which roughly controls the material, with some noise added on top ("heightmap with noise")
- Material Elevation Ranges, which control where the materials start and end

This allows for some coarse control over terrain composition. For example, you can have one corner of the terrain be a sandy beach and the rest of the coastline be rocky. Or you can have the peaks of your mountains be covered with snow. However, artistic control is limited due to the gradient imposed by the material elevation ranges, and the unpredictability of the noise.

A terrain painting option would allow for more control over the terrain composition. The first step to getting that working is the paintmap.

## How to activate the local paintmap

The local paintmap is a good way to assess the quality of the PBR terrain paintmap. By default, the newly created local paintmap inherits its composition (i.e. where the grass and dirt goes) from the existing terrain. This allows for a direct comparison with the terrain heightmap-with-noise shader.

Activating the local paintmap is similar to [applying local PBR terrain via the debug settings](https://wiki.secondlife.com/wiki/PBR_Terrain#How_to_apply_PBR_Terrain), but with a couple extra steps.

You will need:

- Four fullperm PBR material items to copy UUIDs from
- A region with a good variation of elevations which showcase the four composition layers (no special permissions needed)

Open the Debug Settings menu (Advanced > Show Debug Settings) and search for "terrain". The following relevant options are available:

- LocalTerrainAsset1
- LocalTerrainAsset1
- LocalTerrainAsset3
- LocalTerrainAsset4
- LocalTerrainPaintEnabled
- TerrainPaintBitDepth
- TerrainPaintResolution

By setting LocalTerrainAsset1, etc to valid material IDs, you will override the terrain to use those materials.

The next step is to "bake" the terrain into a paintmap (Develop > Terrain > Create Local Paintmap). This will *automatically* set LocalTerrainPaintEnabled to true. **WARNING:** LocalTerrainPaintEnabled will *not* do anything until one of LocalTerrainAsset1, etc is set.

You are now looking at the same terrain, but rendered as a paintmap.

To compare the quality of the paintmap version and the heightmap-with-noise version, toggle LocalTerrainPaintEnabled in Debug Settings.

To change the bit depth and/or resolution of the paintmap, change TerrainPaintBitDepth and TerrainPaintResolution as desired, then "re-bake" the paintmap (Develop > Terrain > Create Local Paintmap).
