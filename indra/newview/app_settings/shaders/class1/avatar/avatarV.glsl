void default_scatter(vec3 viewVec, vec3 lightDir);
vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseCol);
mat4 getSkinnedTransform();
vec2 getScatterCoord(vec3 viewVec, vec3 lightDir);

attribute vec4 materialColor;

void main()
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
				
	vec4 pos;
	vec3 norm;
	
	mat4 trans = getSkinnedTransform();
	pos.x = dot(trans[0], gl_Vertex);
	pos.y = dot(trans[1], gl_Vertex);
	pos.z = dot(trans[2], gl_Vertex);
	pos.w = 1.0;
	
	norm.x = dot(trans[0].xyz, gl_Normal);
	norm.y = dot(trans[1].xyz, gl_Normal);
	norm.z = dot(trans[2].xyz, gl_Normal);
	norm = normalize(norm);
		
	gl_Position = gl_ProjectionMatrix * pos;
	 
	//gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	
	default_scatter(pos.xyz, gl_LightSource[0].position.xyz);

	vec4 color = calcLighting(pos.xyz, norm, materialColor, gl_Color);
	gl_FrontColor = color; 

}
