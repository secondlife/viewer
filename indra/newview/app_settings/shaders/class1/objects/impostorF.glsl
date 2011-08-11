/** 
 * @file impostorF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
uniform float minimum_alpha;
uniform float maximum_alpha;

uniform sampler2D diffuseMap;

void main()
{
	vec4 color = texture2D(diffuseMap,gl_TexCoord[0].xy);
	
	if (color.a < minimum_alpha || color.a > maximum_alpha)
	{
		discard;
	}

	gl_FragColor = color;
}
