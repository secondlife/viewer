/**
 * @file emissiveV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
attribute vec4 position;
attribute float emissive;
attribute vec2 texcoord0;

void calcAtmospherics(vec3 inPositionEye);

varying float vary_texture_index;

void main()
{
	//transform vertex
	vary_texture_index = position.w;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(texcoord0,0,1);
	
	vec4 pos = (gl_ModelViewMatrix * vec4(position.xyz, 1.0));
	calcAtmospherics(pos.xyz);

	gl_FrontColor = vec4(0,0,0,emissive);

	gl_FogFragCoord = pos.z;
}
