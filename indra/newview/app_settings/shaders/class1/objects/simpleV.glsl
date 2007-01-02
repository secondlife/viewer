vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseCol);
void default_scatter(vec3 viewVec, vec3 lightDir);

attribute vec4 materialColor;

void main()
{
	//transform vertex
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	
	vec3 pos = (gl_ModelViewMatrix * gl_Vertex).xyz;
	vec3 norm = normalize(gl_NormalMatrix * gl_Normal);
	
	default_scatter(pos, gl_LightSource[0].position.xyz);

	vec4 color = calcLighting(pos, norm, materialColor, gl_Color);
	gl_FrontColor = color;

	gl_FogFragCoord = pos.z;
}
