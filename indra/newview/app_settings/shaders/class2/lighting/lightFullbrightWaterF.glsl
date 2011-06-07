/** 
 * @file lightFullbrightWaterF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


vec4 diffuseLookup(vec2 texcoord);

vec3 fullbrightAtmosTransport(vec3 light);
vec4 applyWaterFog(vec4 color);

void fullbright_lighting_water()
{
	vec4 color = diffuseLookup(gl_TexCoord[0].xy) * gl_Color;

	color.rgb = fullbrightAtmosTransport(color.rgb);
	
	gl_FragColor = applyWaterFog(color);
}

