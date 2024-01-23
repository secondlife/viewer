Textures imported via Build->Upload->Material that have an all opaque (255) alpha channel should have their alpha channel removed before upload.

1. Make 4 images that have different colors but all 255 alpha channels
2. Upload them all using Build->Upload->Material, with one in each of the material texture slots
3. Verify that using the textures as a blinn-phong diffuse map does not make the corresponding face render in the alpha pass (face should stay visible after disabling alpha pass by unchecking Advanced->Render Types->Alpha).
