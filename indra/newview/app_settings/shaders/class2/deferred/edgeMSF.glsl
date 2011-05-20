/** 
 * @file edgeF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_texture_multisample : enable

uniform sampler2DMS depthMap;
uniform sampler2DMS normalMap;

varying vec2 vary_fragcoord;

uniform float depth_cutoff;
uniform float norm_cutoff;

uniform mat4 inv_proj;
uniform vec2 screen_res;

float getDepth(float z)
{
	z = z*2.0-1.0;
	vec4 ndc = vec4(0.0, 0.0, z, 1.0);
	vec4 p = inv_proj*ndc;
	return p.z/p.w;
}

void main() 
{

	float e = 0;
	
	ivec2 itc = ivec2(vary_fragcoord.xy);

	for (int i = 0; i < samples; i++)
	{	
		vec3 norm = texelFetch(normalMap, itc, i).xyz;
		norm = vec3((norm.xy-0.5)*2.0,norm.z); // unpack norm
		float depth = getDepth(texelFetch(depthMap, itc, i).r);
	
		vec2 tc = vary_fragcoord.xy;
	
		float sc = 0.75;
	
		vec2 de;
		de.x = (depth-getDepth(tc+vec2(sc, sc))) + (depth-getDepth(tc+vec2(-sc, -sc)));
		de.y = (depth-getDepth(tc+vec2(-sc, sc))) + (depth-getDepth(tc+vec2(sc, -sc)));
		de /= depth;
		de *= de;
		de = step(depth_cutoff, de);
	
		vec2 ne;
		vec3 nexnorm = texture2DRect(normalMap, tc+vec2(-sc,-sc)).rgb;
		nexnorm = vec3((nexnorm.xy-0.5)*2.0,nexnorm.z); // unpack norm
		ne.x = dot(nexnorm, norm);
		vec3 neynorm = texture2DRect(normalMap, tc+vec2(sc,sc)).rgb;
		neynorm = vec3((neynorm.xy-0.5)*2.0,neynorm.z); // unpack norm
		ne.y = dot(neynorm, norm);
	
		ne = 1.0-ne;
	
		ne = step(norm_cutoff, ne);

		e += dot(de,de)+dot(ne,ne);
	}

	e /= samples;
	
	gl_FragColor.a = e;
}
