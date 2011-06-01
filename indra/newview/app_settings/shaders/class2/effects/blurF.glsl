/**
 * @file blurf.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

uniform sampler2DRect RenderTexture;
uniform float bloomStrength;

varying vec4 gl_TexCoord[gl_MaxTextureCoords];
void main(void) 
{
	float blurWeights[7];
	blurWeights[0] = 0.05;
	blurWeights[1] = 0.1;
	blurWeights[2] = 0.2;
	blurWeights[3] = 0.3;
	blurWeights[4] = 0.2;
	blurWeights[5] = 0.1;
	blurWeights[6] = 0.05;
	
	vec3 color = vec3(0,0,0);
	for (int i = 0; i < 7; i++){
		color += vec3(texture2DRect(RenderTexture, gl_TexCoord[i].st)) * blurWeights[i];
	}

	color *= bloomStrength;

	gl_FragColor = vec4(color, 1.0);
}
