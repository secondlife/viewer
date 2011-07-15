/**
 * @file extractF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


uniform sampler2DRect RenderTexture;
uniform float extractLow;
uniform float extractHigh;
uniform vec3 lumWeights;

void main(void) 
{
	/// Get scene color
	vec3 color = vec3(texture2DRect(RenderTexture, gl_TexCoord[0].st));
	
	/// Extract luminance and scale up by night vision brightness
	float lum = smoothstep(extractLow, extractHigh, dot(color, lumWeights));

	gl_FragColor = vec4(vec3(lum), 1.0);
}
