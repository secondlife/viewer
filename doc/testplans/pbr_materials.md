# PBR Materials

## KHR Texture Transforms

Texture repeats for PBR materials on prims are based on the [KHR\_texture\_transform](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_texture_transform) spec, and thus should be expected to behave according to the spec. We currently suport offset, rotation, and scale from the spec. texCoord is not currently supported.

PBR materials should have approximately correct lighting based on the normal texture:

- With default texture transforms, assuming the prim or model has correct normals and tangents
- With a texture transform applied, especially rotation or negative scale
- With a texture animation applied via `llSetTextureAnim`, especially a rotation animation
    - Note: Texture animations are not guaranteed to loop when a PBR texture transform is applied
