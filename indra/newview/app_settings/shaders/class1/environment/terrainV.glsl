/** 
 * @file terrainV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
attribute vec3 position;
attribute vec3 normal;
attribute vec4 diffuse_color;
attribute vec2 texcoord0;
attribute vec2 texcoord1;
attribute vec2 texcoord2;
attribute vec2 texcoord3;


vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseCol);

vec4 texgen_object(vec4  vpos, vec4 tc, mat4 mat, vec4 tp0, vec4 tp1)
{
	vec4 tcoord;
	
	tcoord.x = dot(vpos, tp0);
	tcoord.y = dot(vpos, tp1);
	tcoord.z = tc.z;
	tcoord.w = tc.w;
	
	tcoord = mat * tcoord; 
	
	return tcoord; 
}

void main()
{
	//transform vertex
	gl_Position = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
			
	vec4 pos = gl_ModelViewMatrix * position;
	vec3 norm = normalize(gl_NormalMatrix * normal);
	
	vec4 color = calcLighting(pos.xyz, norm, vec4(1,1,1,1), diffuse_color);
	
	gl_FrontColor = color;
	
	gl_TexCoord[0] = texgen_object(vec4(position.xyz, 1.0),vec4(texcoord0,0,1),gl_TextureMatrix[0],gl_ObjectPlaneS[0],gl_ObjectPlaneT[0]);
	gl_TexCoord[1] = gl_TextureMatrix[1]*vec4(texcoord1,0,1);
	gl_TexCoord[2] = texgen_object(vec4(position.xyz, 1.0),vec4(texcoord2,0,1),gl_TextureMatrix[2],gl_ObjectPlaneS[2],gl_ObjectPlaneT[2]);
	gl_TexCoord[3] = gl_TextureMatrix[3]*vec4(texcoord3,0,1);
}
