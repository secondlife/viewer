vec4 calcLightingSpecular(vec3 pos, vec3 norm, vec4 color, inout vec4 specularColor, vec3 baseCol);
void default_scatter(vec3 viewVec, vec3 lightDir);

attribute vec4 materialColor;
attribute vec4 specularColor;

void main()
{
	//transform vertex
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	
	vec3 pos = (gl_ModelViewMatrix * gl_Vertex).xyz;
	vec3 norm = normalize(gl_NormalMatrix * gl_Normal);
	
	default_scatter(pos.xyz, gl_LightSource[0].position.xyz);
	vec4 specular = specularColor;
	vec4 color = calcLightingSpecular(pos, norm, materialColor, specular, gl_Color.rgb);
			
	gl_FrontColor = color;
	gl_FogFragCoord = pos.z;
}

