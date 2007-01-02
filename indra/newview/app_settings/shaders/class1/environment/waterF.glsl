void water_lighting(inout vec3 diff);

uniform samplerCube environmentMap;
uniform sampler2D diffuseMap;
uniform sampler2D bumpMap;

varying vec4 specular;

void main() 
{
	vec4 depth = texture2D(diffuseMap, gl_TexCoord[0].xy);
	vec4 diff = texture2D(bumpMap, gl_TexCoord[1].xy);
	vec3 ref = textureCube(environmentMap, gl_TexCoord[2].xyz).rgb;
	
	diff.rgb *= depth.rgb;
		
	vec3 col = mix(diff.rgb, ref, specular.a)+specular.rgb*diff.rgb;
	
	water_lighting(col.rgb); 
	gl_FragColor.rgb = col.rgb;
	gl_FragColor.a = (gl_Color.a+depth.a)*0.5;	
}
