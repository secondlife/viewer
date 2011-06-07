/** 
 * @file fullbrightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


#extension GL_ARB_texture_rectangle : enable

varying float vary_texture_index;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;
uniform sampler2D tex5;
uniform sampler2D tex6;
uniform sampler2D tex7;

vec4 textureLookup(vec2 texcoord)
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

vec3 fullbrightAtmosTransport(vec3 light);
vec3 fullbrightScaleSoftClip(vec3 light);


void main() 
{
	float shadow = 1.0;

	vec4 color = textureLookup(gl_TexCoord[0].xy)*gl_Color;
	
	color.rgb = fullbrightAtmosTransport(color.rgb);

	color.rgb = fullbrightScaleSoftClip(color.rgb);

	gl_FragColor = color;
}

