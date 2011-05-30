/** 
 * @file diffuseIndexedF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

varying float vary_texture_index;
varying vec3 vary_normal;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;
uniform sampler2D tex5;
uniform sampler2D tex6;
uniform sampler2D tex7;

vec4 diffuseLookup(vec2 texcoord)
{
	switch (int(vary_texture_index+0.25))
	{
		case 0: return texture2D(tex0, texcoord);
		case 1: return texture2D(tex1, texcoord);
		case 2: return texture2D(tex2, texcoord);
		case 3: return texture2D(tex3, texcoord);
		case 4: return texture2D(tex4, texcoord);
		case 5: return texture2D(tex5, texcoord);
		case 6: return texture2D(tex6, texcoord);
		case 7: return texture2D(tex7, texcoord);
	}

	return vec4(0,0,0,0);
}

void main() 
{
	vec3 col = gl_Color.rgb * diffuseLookup(gl_TexCoord[0].xy).rgb;

	//col = vec3(vary_texture_index*0.25, 0, 0);
	
	gl_FragData[0] = vec4(col, 0.0);
	gl_FragData[1] = gl_Color.aaaa; // spec
	//gl_FragData[1] = vec4(vec3(gl_Color.a), gl_Color.a+(1.0-gl_Color.a)*gl_Color.a); // spec - from former class3 - maybe better, but not so well tested
	vec3 nvn = normalize(vary_normal);
	gl_FragData[2] = vec4(nvn.xy * 0.5 + 0.5, nvn.z, 0.0);
}
