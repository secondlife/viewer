/** 
 * @file bumpV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


varying vec3 vary_mat0;
varying vec3 vary_mat1;
varying vec3 vary_mat2;

void main()
{
	//transform vertex
	gl_Position = ftransform(); 
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	
	vec3 n = normalize(gl_NormalMatrix * gl_Normal);
	vec3 b = normalize(gl_NormalMatrix * gl_MultiTexCoord2.xyz);
	vec3 t = cross(b, n);
	
	vary_mat0 = vec3(t.x, b.x, n.x);
	vary_mat1 = vec3(t.y, b.y, n.y);
	vary_mat2 = vec3(t.z, b.z, n.z);
	
	gl_FrontColor = gl_Color;
}
