/** 
 * @file avatarV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseCol);
mat4 getSkinnedTransform();
void calcAtmospherics(vec3 inPositionEye);

attribute vec4 clothing; //4

attribute vec4 gWindDir;		//7
attribute vec4 gSinWaveParams; //3
attribute vec4 gGravity;		//5

const vec4 gMinMaxConstants = vec4(1.0, 0.166666, 0.0083143, .00018542);	 // #minimax-generated coefficients
const vec4 gPiConstants	= vec4(0.159154943, 6.28318530, 3.141592653, 1.5707963); //	# {1/2PI, 2PI, PI, PI/2}

void main()
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
		
	vec4 pos;
	mat4 trans = getSkinnedTransform();
		
	vec3 norm;
	norm.x = dot(trans[0].xyz, gl_Normal);
	norm.y = dot(trans[1].xyz, gl_Normal);
	norm.z = dot(trans[2].xyz, gl_Normal);
	norm = normalize(norm);
		
	//wind
	vec4 windEffect;
	windEffect = vec4(dot(norm, gWindDir.xyz));	
	pos.x = dot(trans[2].xyz, gl_Vertex.xyz);
	windEffect.xyz = pos.x * vec3(0.015, 0.015, 0.015)
						+ windEffect.xyz;
	windEffect.w = windEffect.w * 2.0 + 1.0;				// move wind offset value to [-1, 3]
	windEffect.w = windEffect.w*gWindDir.w;					// modulate wind strength 
	
	windEffect.xyz = windEffect.xyz*gSinWaveParams.xyz
						+vec3(gSinWaveParams.w);			// use sin wave params to scale and offset input

		
	//reduce to period of 2 PI
	vec4 temp1, temp0, temp2, offsetPos;
	temp1.xyz = windEffect.xyz * gPiConstants.x;			// change input as multiple of [0-2PI] to [0-1]
	temp0.y = mod(temp1.x,1.0);	
	windEffect.x = temp0.y * gPiConstants.y;				// scale from [0,1] to [0, 2PI]
	temp1.z = temp1.z - gPiConstants.w;						// shift normal oscillation by PI/2
	temp0.y = mod(temp1.z,1.0);
	
	windEffect.z = temp0.y * gPiConstants.y;				// scale from [0,1] to [0, 2PI]
	windEffect.xyz = windEffect.xyz + vec3(-3.141592);		// offset to [-PI, PI]
															

	//calculate sinusoid
	vec4 sinWave;
	temp1 = windEffect*windEffect;
	sinWave = -temp1 * gMinMaxConstants.w 
				+ vec4(gMinMaxConstants.z);					// y = -(x^2)/7! + 1/5!
	sinWave = sinWave * -temp1 + vec4(gMinMaxConstants.y);	// y = -(x^2) * (-(x^2)/7! + 1/5!) + 1/3!
	sinWave = sinWave * -temp1 + vec4(gMinMaxConstants.x);	// y = -(x^2) * (-(x^2) * (-(x^2)/7! + 1/5!) + 1/3!) + 1
	sinWave = sinWave * windEffect;							// y = x * (-(x^2) * (-(x^2) * (-(x^2)/7! + 1/5!) + 1/3!) + 1)
	
	// sinWave.x holds sin(norm . wind_direction) with primary frequency
	// sinWave.y holds sin(norm . wind_direction) with secondary frequency
	// sinWave.z hold cos(norm . wind_direction) with primary frequency
	sinWave.xyz = sinWave.xyz * gWindDir.w 
				+ vec3(windEffect.w);						// multiply by wind strength in gWindDir.w [-wind, wind]

	// add normal facing bias offset [-wind,wind] -> [-wind - .25, wind + 1]
	temp1 = vec4(dot(norm, gGravity.xyz));					// how much is this normal facing in direction of gGravity?
	temp1 = min(temp1, vec4(0.2,0.0,0.0,0.0));				// clamp [-1, 1] to [-1, 0.2]
	temp1 = temp1*vec4(1.5,0.0,0.0,0.0);					// scale from [-1,0.2] to [-1.5, 0.3]
	sinWave.x = sinWave.x + temp1.x;						// add gGravity effect to sinwave (only primary frequency)
	sinWave.xyz = sinWave.xyz * clothing.w;					// modulate by clothing coverage
	
	sinWave.xyz = max(sinWave.xyz, vec3(-1.0, -1.0, -1.0));	// clamp to underlying body shape
	offsetPos = clothing * sinWave.x;						// multiply wind effect times clothing displacement
	temp2 = gWindDir*sinWave.z + vec4(norm,0);				// calculate normal offset due to wind oscillation
	offsetPos = vec4(1.0,1.0,1.0,0.0)*offsetPos+gl_Vertex;	// add to offset vertex position, and zero out effect from w
	norm += temp2.xyz*2.0;									// add sin wave effect on normals (exaggerated)
	
	//add "backlighting" effect
	float colorAcc;
	colorAcc = 1.0 - clothing.w;
	norm.z -= colorAcc * 0.2;
	
	//renormalize normal (again)
	norm = normalize(norm);

	pos.x = dot(trans[0], offsetPos);
	pos.y = dot(trans[1], offsetPos);
	pos.z = dot(trans[2], offsetPos);
	pos.w = 1.0;

	calcAtmospherics(pos.xyz);
	
	vec4 color = calcLighting(pos.xyz, norm, gl_Color, vec4(0.0));			
	gl_FrontColor = color; 
					
	gl_Position = gl_ProjectionMatrix * pos;
	
	
	gl_TexCoord[2] = vec4(pos.xyz, 1.0);

}
