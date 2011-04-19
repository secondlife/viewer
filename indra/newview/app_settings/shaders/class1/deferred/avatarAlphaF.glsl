/** 
 * @file avatarAlphaF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */

#version 120

uniform sampler2D diffuseMap;
uniform sampler2DShadow shadowMap0;
uniform sampler2DShadow shadowMap1;
uniform sampler2DShadow shadowMap2;
uniform sampler2DShadow shadowMap3;
uniform sampler2D noiseMap;

uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;

vec3 atmosLighting(vec3 light);
vec3 scaleSoftClip(vec3 light);

varying vec3 vary_ambient;
varying vec3 vary_directional;
varying vec4 vary_position;
varying vec3 vary_normal;

void main() 
{
	float shadow = 1.0;
	vec4 pos = vary_position;
	vec3 norm = normalize(vary_normal);
	
	vec3 nz = texture2D(noiseMap, gl_FragCoord.xy/128.0).xyz;

	if (pos.z > -shadow_clip.w)
	{	
		
		if (pos.z < -shadow_clip.z)
		{
			vec4 lpos = shadow_matrix[3]*pos;
			shadow = shadow2DProj(shadowMap3, lpos).x;
		}
		else if (pos.z < -shadow_clip.y)
		{
			vec4 lpos = shadow_matrix[2]*pos;
			shadow = shadow2DProj(shadowMap2, lpos).x;
		}
		else if (pos.z < -shadow_clip.x)
		{
			vec4 lpos = shadow_matrix[1]*pos;
			shadow = shadow2DProj(shadowMap1, lpos).x;
		}
		else
		{
			vec4 lpos = shadow_matrix[0]*pos;
			shadow = shadow2DProj(shadowMap0, lpos).x;
		}
	}
	
	
	vec4 col = vec4(vary_ambient + vary_directional*shadow, gl_Color.a);	
	vec4 color = texture2D(diffuseMap, gl_TexCoord[0].xy) * col;
	
	color.rgb = atmosLighting(color.rgb);

	color.rgb = scaleSoftClip(color.rgb);

	gl_FragColor = color;
}
