attribute vec4 materialColor;

void main()
{
	//transform vertex
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	vec3 pos = (gl_ModelViewMatrix * gl_Vertex).xyz;
	pos = normalize(pos);
	float d = dot(pos, normalize(gl_NormalMatrix * gl_Normal));
	d *= d;
	d = 1.0 - d;
	d *= d;
		
	d = min(d, materialColor.a*2.0);
			
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_FrontColor.rgb = materialColor.rgb;
	gl_FrontColor.a = max(d, materialColor.a);
}

