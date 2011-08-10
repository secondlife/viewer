/** 
 * @file diffuseAlphaMaskNoColorF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 

uniform float minimum_alpha;
uniform float maximum_alpha;

uniform sampler2D diffuseMap;

varying vec3 vary_normal;

void main() 
{
	vec4 col = texture2D(diffuseMap, gl_TexCoord[0].xy);
	
	if (col.a < minimum_alpha || col.a > maximum_alpha)
	{
		discard;
	}

	gl_FragData[0] = vec4(col.rgb, 0.0);
	gl_FragData[1] = vec4(0,0,0,0); // spec
	vec3 nvn = normalize(vary_normal);
	gl_FragData[2] = vec4(nvn.xy * 0.5 + 0.5, nvn.z, 0.0);
}

