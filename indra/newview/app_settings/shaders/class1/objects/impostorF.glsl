/** 
 * @file impostorF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
uniform float minimum_alpha;
uniform float maximum_alpha;

vec3 fullbrightAtmosTransport(vec3 light);
vec3 fullbrightScaleSoftClip(vec3 light);

uniform sampler2D diffuseMap;

void main()
{
	vec4 color = texture2D(diffuseMap,gl_TexCoord[0].xy) * gl_Color;
	
	if (color.a < minimum_alpha || color.a > maximum_alpha)
	{
		discard;
	}

	gl_FragColor = color;
}
