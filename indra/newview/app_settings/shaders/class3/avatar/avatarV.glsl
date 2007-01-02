vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec3 baseCol);
mat4 getSkinnedTransform();
void default_scatter(vec3 viewVec, vec3 lightDir);

attribute vec4 materialColor; //2

attribute vec4 binormal; //6
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
		
	vec3 binorm;
	binorm.x = dot(trans[0].xyz, binormal.xyz);
	binorm.y = dot(trans[1].xyz, binormal.xyz);
	binorm.z = dot(trans[2].xyz, binormal.xyz);
	norm = normalize(norm);
	
	//wind
	vec4 windEffect;
	windEffect = vec4(dot(norm, gWindDir.xyz));				//	DP3 windEffect, blendNorm, gWindDir;
	pos.x = dot(trans[2].xyz, gl_Vertex.xyz);				//	DP3 blendPos.x, blendMatZ, iPos;
	windEffect.xyz = pos.x * vec3(0.015, 0.015, 0.015)
						+ windEffect.xyz;					//	MAD windEffect.xyz, blendPos.x, {0.015, 0.015, 0.015, 0}, windEffect;
	windEffect.w = windEffect.w * 2.0 + 1.0;				//	MAD	windEffect.w, windEffect, {0, 0, 0, 2}, {0, 0, 0, 1};	# move wind offset value to [-1, 3]
	windEffect.w = windEffect.w*gWindDir.w;					//	MUL windEffect.w, windEffect, gWindDir;								# modulate wind strength 
	
	windEffect.xyz = windEffect.xyz*gSinWaveParams.xyz
						+vec3(gSinWaveParams.w);			//	MAD windEffect.xyz, windEffect, gSinWaveParams, gSinWaveParams.w;		# use sin wave params to scale and offset input

		
	//reduce to period of 2 PI
	vec4 temp1, temp0, temp2, offsetPos;
	temp1.xyz = windEffect.xyz * gPiConstants.x;			//	MUL    temp1.xyz, windEffect, gPiConstants.x;						# change input as multiple of [0-2PI] to [0-1]
	temp0.y = mod(temp1.x,1.0);								//	EXP    temp0, temp1.x;												# find mod(x, 1)
	windEffect.x = temp0.y * gPiConstants.y;				//	MUL    windEffect.x, temp0.y, gPiConstants.y;						# scale from [0,1] to [0, 2PI]
	temp1.z = temp1.z - gPiConstants.w;						//	ADD    temp1.z, temp1.z, -gPiConstants.w;							# shift normal oscillation by PI/2
	temp0.y = mod(temp1.z,1.0);								//	EXP    temp0, temp1.z;												# find mod(x, 1)
	
	windEffect.z = temp0.y * gPiConstants.y;				//	MUL    windEffect.z, temp0.y, gPiConstants.y;						# scale from [0,1] to [0, 2PI]
	windEffect.xyz = windEffect.xyz + vec3(-3.141592);		//	# offset to [-PI, PI]
															//	ADD    windEffect.xyz, windEffect, {-3.141592, -3.141592, -3.141592, -3.141592};

	//calculate sinusoid
	vec4 sinWave;
	temp1 = windEffect*windEffect;							//	MUL    temp1,    windEffect, windEffect;							# x^2
	sinWave = -temp1 * gMinMaxConstants.w 
				+ vec4(gMinMaxConstants.z);					//	MAD    sinWave, -temp1, gMinMaxConstants.w, gMinMaxConstants.z;		# y = -(x^2)/7! + 1/5!
	sinWave = sinWave * -temp1 + vec4(gMinMaxConstants.y);	//	MAD    sinWave, sinWave, -temp1, gMinMaxConstants.y;				# y = -(x^2) * (-(x^2)/7! + 1/5!) + 1/3!
	sinWave = sinWave * -temp1 + vec4(gMinMaxConstants.x);	//	MAD    sinWave, sinWave, -temp1, gMinMaxConstants.x;				# y = -(x^2) * (-(x^2) * (-(x^2)/7! + 1/5!) + 1/3!) + 1
	sinWave = sinWave * windEffect;							//	MUL    sinWave, sinWave, windEffect;								# y = x * (-(x^2) * (-(x^2) * (-(x^2)/7! + 1/5!) + 1/3!) + 1)
	
	// sinWave.x holds sin(norm . wind_direction) with primary frequency
	// sinWave.y holds sin(norm . wind_direction) with secondary frequency
	// sinWave.z hold cos(norm . wind_direction) with primary frequency
	sinWave.xyz = sinWave.xyz * gWindDir.w 
				+ vec3(windEffect.w);						//	MAD sinWave.xyz, sinWave, gWindDir.w, windEffect.w;					# multiply by wind strength in gWindDir.w [-wind, wind]

	// add normal facing bias offset [-wind,wind] -> [-wind - .25, wind + 1]
	temp1 = vec4(dot(norm, gGravity.xyz));					//	DP3 temp1, blendNorm, gGravity;										# how much is this normal facing in direction of gGravity?
	temp1 = min(temp1, vec4(0.2,0.0,0.0,0.0));				//	MIN temp1, temp1, {0.2, 0, 0, 0};									# clamp [-1, 1] to [-1, 0.2]
	temp1 = temp1*vec4(1.5,0.0,0.0,0.0);					//	MUL temp1, temp1, {1.5, 0, 0, 0};									# scale from [-1,0.2] to [-1.5, 0.3]
	sinWave.x = sinWave.x + temp1.x;						//	ADD sinWave.x, sinWave, temp1;										# add gGravity effect to sinwave (only primary frequency)
	sinWave.xyz = sinWave.xyz * clothing.w;					//	MUL sinWave.xyz, sinWave, iClothing.w;								# modulate by clothing coverage
	
	sinWave.xyz = max(sinWave.xyz, vec3(-1.0, -1.0, -1.0));	//	MAX sinWave.xyz, sinWave, {-1, -1, -1, -1};							# clamp to underlying body shape
	offsetPos = clothing * sinWave.x;						//	MUL offsetPos, iClothing, sinWave.x;								# multiply wind effect times clothing displacement
	temp2 = gWindDir*sinWave.z + vec4(norm,0);				//	MAD temp2, gWindDir, sinWave.z, blendNorm;							# calculate normal offset due to wind oscillation
	offsetPos = vec4(1.0,1.0,1.0,0.0)*offsetPos+gl_Vertex;	//	MAD offsetPos, {1.0, 1.0, 1.0, 0.0}, offsetPos, iPos;				# add to offset vertex position, and zero out effect from w
	norm += temp2.xyz*2.0;									//	MAD blendNorm, temp2, {2, 2, 2, 2}, blendNorm;						# add sin wave effect on normals (exaggerated)
	
	//add "backlighting" effect
	float colorAcc;
	colorAcc = 1.0 - clothing.w;							//	SUB colorAcc, {1, 1, 1, 1}, iClothing;
	norm.z -= colorAcc * 0.2;								//	MAD blendNorm, colorAcc.w, {0, 0, -0.2, 0}, blendNorm;
	
	//renormalize normal (again)
	norm = normalize(norm);									//	DP3 divisor.w, blendNorm, blendNorm;
															// RSQ divisor.xyz, divisor.w;
															// MUL blendNorm.xyz, blendNorm, divisor;

	//project binormal to normal plane to ensure orthogonality
	temp2 = vec4(dot(norm, binorm));						//	DP3 temp2, blendNorm, blendBinorm;
	binorm = binorm - temp2.xyz;							//	SUB blendBinorm, blendBinorm, temp2;

	//renormalize binormal
	binorm = normalize(binorm);								//	DP3 divisor.w, blendBinorm, blendBinorm;
															//	RSQ divisor.xyz, divisor.w;
															//	MUL blendBinorm.xyz, blendBinorm, divisor;

	pos.x = dot(trans[0], offsetPos);
	pos.y = dot(trans[1], offsetPos);
	pos.z = dot(trans[2], offsetPos);
	pos.w = 1.0;
	
	vec4 color = calcLighting(pos.xyz, norm, materialColor, gl_Color.rgb);			
	gl_FrontColor = color; 
					
	gl_Position = gl_ProjectionMatrix * pos;
	
	vec3 N = norm;
	vec3 B = binorm;
	vec3 T = cross(N,B);
	
	//gl_TexCoord[1].xy = gl_MultiTexCoord0.xy + 1.0/512.0 * vec2(dot(T,gl_LightSource[0].position.xyz),
	//												dot(B,gl_LightSource[0].position.xyz));

	gl_TexCoord[2] = vec4(pos.xyz, 1.0);
	default_scatter(pos.xyz, gl_LightSource[0].position.xyz);
										 
}