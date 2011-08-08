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

varying vec3 vary_normal;

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
			
	vary_normal = normalize(gl_NormalMatrix * normal);
	
	// Transform and pass tex coords
 	gl_TexCoord[0].xy = texgen_object(vec4(position, 1.0), vec4(texcoord0,0,1), gl_TextureMatrix[0], gl_ObjectPlaneS[0], gl_ObjectPlaneT[0]).xy;
	
	vec4 t = vec4(texcoord1,0,1);
	
	gl_TexCoord[0].zw = t.xy;
	gl_TexCoord[1].xy = t.xy-vec2(2.0, 0.0);
	gl_TexCoord[1].zw = t.xy-vec2(1.0, 0.0);
}
