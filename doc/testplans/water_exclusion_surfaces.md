

Create an "water_exclusion_surface script" by adding the following script to a new script in your inventory.

~~~
default
{
    state_entry()
    {
        // Make this prim a water_exclusion_surface.
        llSetPrimitiveParams([PRIM_BUMP_SHINY, ALL_SIDES, PRIM_SHINY_NONE, PRIM_BUMP_NONE,
            PRIM_COLOR, ALL_SIDES, <1.0, 1.0, 1.0>, 1.0, PRIM_TEXGEN, ALL_SIDES, PRIM_TEXGEN_DEFAULT,
            PRIM_TEXTURE, ALL_SIDES, "e97cf410-8e61-7005-ec06-629eba4cd1fb", ZERO_VECTOR, ZERO_VECTOR, 0.0]);
        
        // Delete ourselves from the prim; we are no longer needed.
        llRemoveInventory(llGetScriptName());
    }
}
~~~

Make an water_exclusion_surface object by dragging the script from inventory onto a new prim. Take the prim to your inventory and name it water_exclusion_surface.

Rez the water_exclusion_surface prim onto a water surface.

Check above the water example:

Doesn't matter the water, if a hole is not present that conforms to the shape of the object per the above water attached screenshot fail this
    
![410886108-319899d7-1f4c-4f69-a07c-e7503529e1b6](https://github.com/user-attachments/assets/b39233dd-e298-4245-a383-4dfc4ae52595)

Check below the water example:

There should be an unrefracted view to the sky or any other "above water" elements as demonstrated in the "below water" example that conforms to the shape of the object.
    
![410886267-e443bbc1-70db-4f86-b534-ff0a5c45c316](https://github.com/user-attachments/assets/d61d7c3d-ef15-4f3a-99c1-53cd9b01bf3a)

Attach an water_exclusion_surface object to your avatar.

If the water_exclusion_surface object works against water, fail. water_exclusion_surface object should never work against water when attached.

Watch another avatar wearing an water_exclusion_surface object.

If the water_exclusion_surface object works against water, fail.

If the rigged mesh water_exclusion_surface object works against water, fail. Rigged water_exclusion_surface object are not to be supported.

Rigged water_exclusion_surface object should appear solid. Their color should be black, and their specular may be present. Rigged water_exclusion_surface object appearance is undefined behaviour - the point is they should not work as water_exclusion_surface object.

HUD - Verify it does not appear to affect rendering in any way, although it will block you clicking into the world.

Mirror (reflective object) - The reflective object will not become invisible because its shininess is set to a texture.

Shininess (specular) - If the specular Blinn Phong texture is not set to blank, the object will not be invisible.

Mirror (Reflection probe) - a reflection probe can be made to also be an water_exclusion_surface object. An water_exclusion_surface object can be made into a Reflection probe, but it will lose its water_exclusion_surface object attribute.

Transparency - Transparency is changed to 0% transparent and becomes an water_exclusion_surface object. Adding any transparency to an water_exclusion_surface object removes its invisiprim attribute.

Glow - the glow parameter does not appear to have any effect on an water_exclusion_surface object.

PBR - If an object has a PBR material it will not change to an water_exclusion_surface object when the water_exclusion_surface script is applied.
