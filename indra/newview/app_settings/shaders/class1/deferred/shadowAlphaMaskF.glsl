/** 
 * @file shadowAlphaMaskF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
uniform float minimum_alpha;
uniform float maximum_alpha;

uniform sampler2D diffuseMap;

varying vec4 post_pos;

void main() 
{
	float alpha = texture2D(diffuseMap, gl_TexCoord[0].xy).a * gl_Color.a;

	if (alpha < minimum_alpha || alpha > maximum_alpha)
	{
		discard;
	}

	gl_FragColor = vec4(1,1,1,1);
	
	gl_FragDepth = max(post_pos.z/post_pos.w*0.5+0.5, 0.0);
}
