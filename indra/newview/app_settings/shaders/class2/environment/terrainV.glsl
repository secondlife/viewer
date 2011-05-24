/**
 * @file terrainV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

void calcAtmospherics(vec3 inPositionEye);

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
	gl_Position = ftransform();

	vec4 pos = gl_ModelViewMatrix * gl_Vertex;
	vec3 norm = normalize(gl_NormalMatrix * gl_Normal);

	/// Potentially better without it for water.
	pos /= pos.w;

	calcAtmospherics((gl_ModelViewMatrix * gl_Vertex).xyz);

	vec4 color = calcLighting(pos.xyz, norm, gl_Color, vec4(0));
	
	gl_FrontColor = color;

	// Transform and pass tex coords
 	gl_TexCoord[0].xy = texgen_object(gl_Vertex, gl_MultiTexCoord0, gl_TextureMatrix[0], gl_ObjectPlaneS[0], gl_ObjectPlaneT[0]).xy;
	
	vec4 t = gl_MultiTexCoord1;
	
	gl_TexCoord[0].zw = t.xy;
	gl_TexCoord[1].xy = t.xy-vec2(2.0, 0.0);
	gl_TexCoord[1].zw = t.xy-vec2(1.0, 0.0);
}

