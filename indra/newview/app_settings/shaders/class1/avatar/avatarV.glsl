/** 
 * @file avatarV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
attribute vec3 position;
attribute vec3 normal;
attribute vec2 texcoord0;

vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseCol);
mat4 getSkinnedTransform();
void calcAtmospherics(vec3 inPositionEye);

void main()
{
	gl_TexCoord[0] = vec4(texcoord0,0,1);
				
	vec4 pos;
	vec3 norm;
	
	vec4 pos_in = vec4(position.xyz, 1.0);

	mat4 trans = getSkinnedTransform();
	pos.x = dot(trans[0], pos_in);
	pos.y = dot(trans[1], pos_in);
	pos.z = dot(trans[2], pos_in);
	pos.w = 1.0;
	
	norm.x = dot(trans[0].xyz, normal);
	norm.y = dot(trans[1].xyz, normal);
	norm.z = dot(trans[2].xyz, normal);
	norm = normalize(norm);
		
	gl_Position = gl_ProjectionMatrix * pos;
	
	//gl_Position = gl_ModelViewProjectionMatrix * position;
	
	gl_FogFragCoord = length(pos.xyz);

	calcAtmospherics(pos.xyz);

	vec4 color = calcLighting(pos.xyz, norm, vec4(1,1,1,1), vec4(0,0,0,0));
	gl_FrontColor = color; 

}


