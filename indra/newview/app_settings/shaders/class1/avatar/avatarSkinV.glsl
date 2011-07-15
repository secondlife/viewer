/** 
 * @file avatarSkinV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


attribute vec4 weight;  //1

uniform vec4 matrixPalette[45];

mat4 getSkinnedTransform()
{
	mat4 ret;
	int i = int(floor(weight.x));
	float x = fract(weight.x);
		
	ret[0] = mix(matrixPalette[i+0], matrixPalette[i+1], x);
	ret[1] = mix(matrixPalette[i+15],matrixPalette[i+16], x);
	ret[2] = mix(matrixPalette[i+30],matrixPalette[i+31], x);
	ret[3] = vec4(0,0,0,1);

	return ret;
}
