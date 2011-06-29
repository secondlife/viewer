/**
 * @file blurV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


uniform vec2 texelSize;
uniform vec2 blurDirection;
uniform float blurWidth;

void main(void)
{
	// Transform vertex
	gl_Position = ftransform();
	
	vec2 blurDelta = texelSize * blurDirection * vec2(blurWidth, blurWidth);
	vec2 s = gl_MultiTexCoord0.st - (blurDelta * 3.0);
	
	// for (int i = 0; i < 7; i++) {
		// gl_TexCoord[i].st = s + (i * blurDelta);
	// }

	// MANUALLY UNROLL
	gl_TexCoord[0].st = s;
	gl_TexCoord[1].st = s + blurDelta;
	gl_TexCoord[2].st = s + (2. * blurDelta);
	gl_TexCoord[3].st = s + (3. * blurDelta);
	gl_TexCoord[4].st = s + (4. * blurDelta);
	gl_TexCoord[5].st = s + (5. * blurDelta);
	gl_TexCoord[6].st = s + (6. * blurDelta);

	// gl_TexCoord[0].st = s;
	// gl_TexCoord[1].st = blurDelta;
}
