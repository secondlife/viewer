/** 
 * @file diffuseV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


varying vec3 vary_normal;
varying float vary_texture_index;

void main()
{
	//transform vertex
	gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0); 
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	
	vary_texture_index = gl_Vertex.w;
	vary_normal = normalize(gl_NormalMatrix * gl_Normal);

	gl_FrontColor = gl_Color;
}
