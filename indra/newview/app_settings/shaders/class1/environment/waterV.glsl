void default_scatter(vec3 viewVec, vec3 lightDir);
vec4 calcLightingSpecular(vec3 pos, vec3 norm, vec4 color, inout vec4 specularColor, vec3 baseCol);
vec2 getScatterCoord(vec3 viewVec, vec3 lightDir);

varying vec4 specular;

vec4 texgen_object(vec4  vpos, vec4 tc, mat4 mat, vec4 tp0, vec4 tp1)
{
	vec4 tcoord;
	
	tcoord.x = dot(vpos, tp0);
	tcoord.y = dot(vpos, tp1);
	tcoord.z = tc.z;
	tcoord.w = tc.w;
	
	tcoord = mat * tcoord; 
	
	return tcoord; 
}

void main()
{
	//transform vertex
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = texgen_object(gl_Vertex, gl_MultiTexCoord1, gl_TextureMatrix[1], gl_ObjectPlaneS[1],gl_ObjectPlaneT[1]);
	
	vec3 pos = (gl_ModelViewMatrix * gl_Vertex).xyz;
	vec3 norm = normalize(gl_NormalMatrix * gl_Normal);
	vec4 spec = gl_Color;
	gl_FrontColor.rgb = calcLightingSpecular(pos, norm, gl_Color, spec, vec3(0.0, 0.0, 0.0)).rgb;			
	gl_FrontColor.a = gl_Color.a;
	specular = spec;
	specular.a = gl_Color.a*0.5;
	vec3 ref = reflect(pos,norm);
	
	gl_TexCoord[2] = gl_TextureMatrix[2]*vec4(ref,1);
		
	default_scatter(pos.xyz, gl_LightSource[0].position.xyz);
}

