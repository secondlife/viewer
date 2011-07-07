/** 
 * @file postgiF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect depthMap;
uniform sampler2DRect normalMap;
uniform sampler2DRect giLightMap;
uniform sampler2D	noiseMap;
uniform sampler2D	giMip;
uniform sampler2DRect edgeMap;


uniform vec2 delta;
uniform float kern_scale;
uniform float gi_edge_weight;
uniform float gi_blur_brightness;

varying vec2 vary_fragcoord;

void main() 
{
	vec2 dlt = kern_scale*delta;
	float defined_weight = 0.0; 
	vec3 col = vec3(0.0); 
	
	float e = 1.0;
	
	for (int i = 1; i < 8; i++)
	{
		vec2 tc = vary_fragcoord.xy + float(i) * dlt;
		
		e = max(e, 0.0);
		float wght = e;
		
	   	col += texture2DRect(giLightMap, tc).rgb*wght;
		defined_weight += wght;
				
		e *= e;
		e -=(texture2DRect(edgeMap, tc.xy-dlt*0.25).a+
			texture2DRect(edgeMap, tc.xy+dlt*0.25).a)*gi_edge_weight;
	}

	e = 1.0;
	
	for (int i = 1; i < 8; i++)
	{
		vec2 tc = vary_fragcoord.xy - float(i) * dlt;
		
		e = max(e,0.0);
		float wght = e;
		
	   	col += texture2DRect(giLightMap, tc).rgb*wght;
		defined_weight += wght;
	
		e *= e;
		e -= (texture2DRect(edgeMap, tc.xy-dlt*0.25).a+
			texture2DRect(edgeMap, tc.xy+dlt*0.25).a)*gi_edge_weight;
		
	}
	
	col /= max(defined_weight, 0.01);

	gl_FragColor.rgb = col * gi_blur_brightness;
}
