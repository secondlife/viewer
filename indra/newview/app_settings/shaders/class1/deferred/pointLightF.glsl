/** 
 * @file pointLightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
 

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect diffuseRect;
uniform sampler2DRect specularRect;
uniform sampler2DRect normalMap;
uniform samplerCube environmentMap;
uniform sampler2D noiseMap;
uniform sampler2D lightFunc;
uniform sampler2DRect depthMap;

uniform vec3 env_mat[3];
uniform float sun_wash;

varying vec4 vary_light;

varying vec4 vary_fragcoord;
uniform vec2 screen_res;

uniform mat4 inv_proj;
uniform vec4 viewport;

vec4 getPosition(vec2 pos_screen)
{
	float depth = texture2DRect(depthMap, pos_screen.xy).r;
	vec2 sc = (pos_screen.xy-viewport.xy)*2.0;
	sc /= viewport.zw;
	sc -= vec2(1.0,1.0);
	vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
	vec4 pos = inv_proj * ndc;
	pos /= pos.w;
	pos.w = 1.0;
	return pos;
}

void main() 
{
	vec4 frag = vary_fragcoord;
	frag.xyz /= frag.w;
	frag.xyz = frag.xyz*0.5+0.5;
	frag.xy *= screen_res;
	
	vec3 pos = getPosition(frag.xy).xyz;
	vec3 lv = vary_light.xyz-pos;
	float dist2 = dot(lv,lv);
	dist2 /= vary_light.w;
	if (dist2 > 1.0)
	{
		discard;
	}
	
	vec3 norm = texture2DRect(normalMap, frag.xy).xyz;
	norm = vec3((norm.xy-0.5)*2.0,norm.z); // unpack norm
	float da = dot(norm, lv);
	if (da < 0.0)
	{
		discard;
	}
	
	norm = normalize(norm);
	lv = normalize(lv);
	da = dot(norm, lv);
	
	float noise = texture2D(noiseMap, frag.xy/128.0).b;
	
	vec3 col = texture2DRect(diffuseRect, frag.xy).rgb;
	float fa = gl_Color.a+1.0;
	float dist_atten = clamp(1.0-(dist2-1.0*(1.0-fa))/fa, 0.0, 1.0);
	float lit = da * dist_atten * noise;
	
	col = gl_Color.rgb*lit*col;

	vec4 spec = texture2DRect(specularRect, frag.xy);
	if (spec.a > 0.0)
	{
		float sa = dot(normalize(lv-normalize(pos)),norm);
		if (sa > 0.0)
		{
			sa = texture2D(lightFunc, vec2(sa, spec.a)).a * min(dist_atten*4.0, 1.0);
			sa *= noise;
			col += da*sa*gl_Color.rgb*spec.rgb;
		}
	}
	
	if (dot(col, col) <= 0.0)
	{
		discard;
	}
		
	gl_FragColor.rgb = col;	
	gl_FragColor.a = 0.0;
}
