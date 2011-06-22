/**
 * @file colorFilterF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


uniform sampler2DRect RenderTexture;
uniform float brightness;
uniform float contrast;
uniform vec3  contrastBase;
uniform float saturation;
uniform vec3  lumWeights;

const float gamma = 2.0;

void main(void) 
{
	vec3 color = vec3(texture2DRect(RenderTexture, gl_TexCoord[0].st));

	/// Modulate brightness
	color *= brightness;

	/// Modulate contrast
	color = mix(contrastBase, color, contrast);

	/// Modulate saturation
	color = mix(vec3(dot(color, lumWeights)), color, saturation);

	gl_FragColor = vec4(color, 1.0);
}
