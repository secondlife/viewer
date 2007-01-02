void terrain_lighting(inout vec3 color);

uniform sampler2D detail0; //0
uniform sampler2D detail1; //2
uniform sampler2D alphaRamp; //1


void main() 
{
	float a = texture2D(alphaRamp, gl_TexCoord[1].xy).a;
	vec3 color = mix(texture2D(detail1, gl_TexCoord[2].xy).rgb,
					 texture2D(detail0, gl_TexCoord[0].xy).rgb,
					 a);

	terrain_lighting(color);
	
	gl_FragColor.rgb = color;
	gl_FragColor.a = texture2D(alphaRamp, gl_TexCoord[3].xy).a;
}
