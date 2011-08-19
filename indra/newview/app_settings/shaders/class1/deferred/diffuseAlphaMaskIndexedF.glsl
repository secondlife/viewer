/** 
 * @file diffuseAlphaMaskIndexedF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
varying vec3 vary_normal;

uniform float minimum_alpha;
uniform float maximum_alpha;

void main() 
{
	vec4 col = diffuseLookup(gl_TexCoord[0].xy) * gl_Color;
	
	if (col.a < minimum_alpha || col.a > maximum_alpha)
	{
		discard;
	}

	gl_FragData[0] = vec4(col.rgb, 0.0);
	gl_FragData[1] = vec4(0,0,0,0);
	vec3 nvn = normalize(vary_normal);
	gl_FragData[2] = vec4(nvn.xy * 0.5 + 0.5, nvn.z, 0.0);
}
