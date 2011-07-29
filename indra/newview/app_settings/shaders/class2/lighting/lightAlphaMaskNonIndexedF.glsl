/** 
 * @file lightAlphaMaskNonIndexedF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 

uniform float minimum_alpha;
uniform float maximum_alpha;


uniform sampler2D diffuseMap;

vec3 atmosLighting(vec3 light);
vec3 scaleSoftClip(vec3 light);

void default_lighting() 
{
	vec4 color = texture2D(diffuseMap,gl_TexCoord[0].xy) * gl_Color;

	if (color.a < minimum_alpha || color.a > maximum_alpha)
	{
		discard;
	}

	color.rgb = atmosLighting(color.rgb);

	color.rgb = scaleSoftClip(color.rgb);

	gl_FragColor = color;
}

