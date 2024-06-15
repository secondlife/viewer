A resident may swap out their sky for an EXR format HDRI for the purposes of previewing how their object would render in Second Life in an environment that matches the supplied HDRI.  This should aid in matching inworld lighting with external tools so artists can know if their content has imported properly.

To load an HDRI, click Develop->Render Tests->HDRI Preview:

![image](https://github.com/secondlife/viewer/assets/23218274/fbdeab5f-dc1f-4406-be19-0c9ee7437b3f)

Choose an EXR image.  A library of publicly available HDRIs can be found here: https://polyhaven.com/hdris

The Personal Lighting floater will open, and the sky will be replaced with the HDRI you chose.  Reflection Probes will reset, and the scene will be illuminated by the HDRI.

Three debug settings affect how the HDRI is displayed:

RenderHDRIExposure - Exposure adjustment of HDRI when previewing an HDRI.  Units are EV.  Sane values would be -10 to 10.
RenderHDRIRotation - Rotation (in degrees) of environment when previewing an HDRI.
RenderHDRISplitScreen - What percentage of screen to render using HDRI vs EEP sky.

Exposure and Rotation should behave similarly to the rotation and exposure controls in Substance Painter.

Split Screen can be used to display an EEP sky side-by-side with an HDRI sky to aid in authoring an EEP sky that matches an HDRI sky.  It is currently expected that EEP sun disc, moon, clouds, and stars do not render when previewing an HDRI, but that may change in the future.


