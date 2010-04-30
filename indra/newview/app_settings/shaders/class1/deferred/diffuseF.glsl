/** 
 * @file diffuseF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

uniform sampler2D diffuseMap;

varying vec3 vary_normal;

void main() 
{
	vec3 col = gl_Color.rgb * texture2D(diffuseMap, gl_TexCoord[0].xy).rgb;
	gl_FragData[0] = vec4(col, 0.0);
	gl_FragData[1] = vec4(gl_Color.aaa*1.5, gl_Color.a); // spec
	//gl_FragData[1] = vec4(vec3(gl_Color.a), gl_Color.a+(1.0-gl_Color.a)*gl_Color.a); // spec - from former class3 - maybe better, but not so well tested
	gl_FragData[2] = vec4(normalize(vary_normal)*0.5+0.5, 0.0);
}
