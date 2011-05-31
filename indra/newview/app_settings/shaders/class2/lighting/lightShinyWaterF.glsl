/** 
 * @file lightShinyWaterF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 



uniform samplerCube environmentMap;

vec3 atmosLighting(vec3 light);
vec4 applyWaterFog(vec4 color);

void shiny_lighting_water()
{
	vec4 color = diffuseLookup(gl_TexCoord[0].xy);
	color.rgb *= gl_Color.rgb;
	
	vec3 envColor = textureCube(environmentMap, gl_TexCoord[1].xyz).rgb;	
	color.rgb = mix(color.rgb, envColor.rgb, gl_Color.a);

	color.rgb = atmosLighting(color.rgb);
	color.a = max(color.a, gl_Color.a);
	gl_FragColor = applyWaterFog(color);
}

