void applyScatter(inout vec3 col);

uniform samplerCube environmentMap;

void main() 
{
	vec3 ref = textureCube(environmentMap, gl_TexCoord[0].xyz).rgb;
			
	applyScatter(ref);
		
	gl_FragColor.rgb = ref;
	gl_FragColor.a = gl_Color.a;
}
