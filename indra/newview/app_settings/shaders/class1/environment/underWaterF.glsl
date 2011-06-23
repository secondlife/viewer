/**
 * @file underWaterF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


uniform sampler2D diffuseMap;
uniform sampler2D bumpMap;   
uniform sampler2D screenTex;

uniform float refScale;
uniform vec4 waterFogColor;

//bigWave is (refCoord.w, view.w);
varying vec4 refCoord;
varying vec4 littleWave;
varying vec4 view;

void main() 
{
	vec4 color;    
	
	//get bigwave normal
	vec3 wavef = texture2D(bumpMap, vec2(refCoord.w, view.w)).xyz*2.0;
    
	//get detail normals
	vec3 dcol = texture2D(bumpMap, littleWave.xy).rgb*0.75;
	dcol += texture2D(bumpMap, littleWave.zw).rgb*1.25;
	    
	//interpolate between big waves and little waves (big waves in deep water)
	wavef = (wavef+dcol)*0.5;

	//crunch normal to range [-1,1]
	wavef -= vec3(1,1,1);
	
	//figure out distortion vector (ripply)   
	vec2 distort = (refCoord.xy/refCoord.z) * 0.5 + 0.5;
	distort = distort+wavef.xy*refScale;
		
	vec4 fb = texture2D(screenTex, distort);
	
	gl_FragColor.rgb = mix(waterFogColor.rgb, fb.rgb, waterFogColor.a * 0.001 + 0.999);
	gl_FragColor.a = fb.a;
}
