void applyScatter(inout vec3 color);

uniform sampler2D diffuseMap;

void default_lighting() 
{
	vec4 color = gl_Color * texture2D(diffuseMap, gl_TexCoord[0].xy);
	//applyScatter(color.rgb);
	gl_FragColor = color;
}

void alpha_lighting() 
{
	vec4 diff = texture2D(diffuseMap, gl_TexCoord[0].xy);
	vec3 color = gl_Color.rgb * diff.rgb;
	applyScatter(color);
	gl_FragColor.rgb = color;
	gl_FragColor.a = diff.a * gl_Color.a;	
}

void water_lighting(inout vec3 diff)
{
	diff = (diff*0.9 + gl_Color.rgb*0.1);
	applyScatter(diff);
}

void terrain_lighting(inout vec3 color)
{
	color.rgb *= gl_Color.rgb;
	applyScatter(color);
}

vec4 getLightColor()
{
	return gl_Color;
}