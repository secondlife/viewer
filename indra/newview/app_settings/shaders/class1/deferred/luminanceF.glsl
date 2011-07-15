/** 
 * @file luminanceF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */



uniform sampler2DRect diffuseMap;

varying vec2 vary_fragcoord;

void main() 
{
	gl_FragColor = texture2DRect(diffuseMap, vary_fragcoord.xy);
}
