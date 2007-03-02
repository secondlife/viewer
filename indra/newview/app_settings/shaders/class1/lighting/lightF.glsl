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
	default_lighting();
}

void water_lighting(inout vec3 diff)
{
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