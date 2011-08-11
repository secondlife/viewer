/** 
 * @file bumpV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
attribute vec3 position;
attribute vec4 diffuse_color;
attribute vec3 normal;
attribute vec2 texcoord0;
attribute vec2 texcoord2;

varying vec3 vary_mat0;
varying vec3 vary_mat1;
varying vec3 vary_mat2;

void main()
{
	//transform vertex
	gl_Position = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0); 
	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(texcoord0,0,1);
	
	vec3 n = normalize(gl_NormalMatrix * normal);
	vec3 b = normalize(gl_NormalMatrix * vec4(texcoord2,0,1).xyz);
	vec3 t = cross(b, n);
	
	vary_mat0 = vec3(t.x, b.x, n.x);
	vary_mat1 = vec3(t.y, b.y, n.y);
	vary_mat2 = vec3(t.z, b.z, n.z);
	
	gl_FrontColor = diffuse_color;
}
