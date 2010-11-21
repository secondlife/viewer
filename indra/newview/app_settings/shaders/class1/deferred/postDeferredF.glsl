/** 
 * @file postDeferredF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect diffuseRect;
uniform sampler2DRect edgeMap;
uniform sampler2DRect depthMap;
uniform sampler2DRect normalMap;
uniform sampler2D bloomMap;

uniform float depth_cutoff;
uniform float norm_cutoff;

uniform mat4 inv_proj;
uniform vec2 screen_res;

varying vec2 vary_fragcoord;

float getDepth(vec2 pos_screen)
{
	float z = texture2DRect(depthMap, pos_screen.xy).a;
	z = z*2.0-1.0;
	vec4 ndc = vec4(0.0, 0.0, z, 1.0);
	vec4 p = inv_proj*ndc;
	return p.z/p.w;
}

void main() 
{
	vec3 norm = texture2DRect(normalMap, vary_fragcoord.xy).xyz;
	norm = vec3((norm.xy-0.5)*2.0,norm.z); // unpack norm
	float depth = getDepth(vary_fragcoord.xy);
	
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
	
	float edge_weight = clamp(dot(de,de)+dot(ne,ne), 0.0, 1.0);
	//edge_weight *= 0.0;
	
	vec4 bloom = texture2D(bloomMap, vary_fragcoord.xy/screen_res);
	vec4 diff = texture2DRect(diffuseRect, vary_fragcoord.xy);
	diff += texture2DRect(diffuseRect, vary_fragcoord.xy+vec2(1,1))*edge_weight;
	diff += texture2DRect(diffuseRect, vary_fragcoord.xy+vec2(-1,-1))*edge_weight;
	diff += texture2DRect(diffuseRect, vary_fragcoord.xy+vec2(-1,1))*edge_weight;
	diff += texture2DRect(diffuseRect, vary_fragcoord.xy+vec2(1,-1))*edge_weight;
	diff += texture2DRect(diffuseRect, vary_fragcoord.xy+vec2(-1,0))*edge_weight;
	diff += texture2DRect(diffuseRect, vary_fragcoord.xy+vec2(1,0))*edge_weight;
	diff += texture2DRect(diffuseRect, vary_fragcoord.xy+vec2(0,1))*edge_weight;
	diff += texture2DRect(diffuseRect, vary_fragcoord.xy+vec2(0,-1))*edge_weight;
	
	diff /= 1.0+edge_weight*8.0;
	
	vec4 blur = texture2DRect(edgeMap, vary_fragcoord.xy);
	
	//gl_FragColor = vec4(edge_weight,edge_weight,edge_weight, 1.0);
	gl_FragColor = diff + bloom;
	//gl_FragColor.r = edge_weight;
	
}
