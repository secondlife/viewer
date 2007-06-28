void default_scatter(vec3 viewVec, vec3 lightDir);

uniform vec4 origin;

void main()
{
	//transform vertex
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	
	vec3 pos = (gl_ModelViewMatrix * gl_Vertex).xyz;
	vec3 norm = normalize(gl_NormalMatrix * gl_Normal);
	
	gl_FrontColor = gl_Color;
	
	vec3 ref = reflect(pos, norm);
	
	vec3 d = pos - origin.xyz;
	float dist = dot(normalize(d), ref);
	vec3 e = d + (ref * max(origin.w-dist, 0.0));
	
	ref = e - origin.xyz;
	
	gl_TexCoord[0] = gl_TextureMatrix[0]*vec4(ref,1.0);
	
	default_scatter(pos.xyz, gl_LightSource[0].position.xyz);
}

