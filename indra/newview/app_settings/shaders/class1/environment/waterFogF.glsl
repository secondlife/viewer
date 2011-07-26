/**
 * @file waterFogF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


vec4 applyWaterFog(vec4 color)
{
	// GL_EXP2 Fog
	//float fog = exp(-gl_Fog.density * gl_Fog.density * gl_FogFragCoord * gl_FogFragCoord);
	// GL_EXP Fog
	// float fog = exp(-gl_Fog.density * gl_FogFragCoord);
	// GL_LINEAR Fog
	float fog = (gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale;
	fog = clamp(fog, 0.0, 1.0);
	color.rgb = mix(gl_Fog.color.rgb, color.rgb, fog);
	return color;
}

