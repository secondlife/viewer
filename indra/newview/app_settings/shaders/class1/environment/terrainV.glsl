/** 
 * @file terrainV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

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
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
			
	vec4 pos = gl_ModelViewMatrix * gl_Vertex;
	vec3 norm = normalize(gl_NormalMatrix * gl_Normal);
	
	vec4 color = calcLighting(pos.xyz, norm, vec4(1,1,1,1), gl_Color);
	
	gl_FrontColor = color;
	
	gl_TexCoord[0] = texgen_object(gl_Vertex,gl_MultiTexCoord0,gl_TextureMatrix[0],gl_ObjectPlaneS[0],gl_ObjectPlaneT[0]);
	gl_TexCoord[1] = gl_TextureMatrix[1]*gl_MultiTexCoord1;
	gl_TexCoord[2] = texgen_object(gl_Vertex,gl_MultiTexCoord2,gl_TextureMatrix[2],gl_ObjectPlaneS[2],gl_ObjectPlaneT[2]);
	gl_TexCoord[3] = gl_TextureMatrix[3]*gl_MultiTexCoord3;
}
