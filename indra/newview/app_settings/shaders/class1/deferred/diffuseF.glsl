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
	vec3 col = texture2D(diffuseMap, gl_TexCoord[0].xy).rgb;
	gl_FragData[0] = vec4(gl_Color.rgb*col, 0.0);
	gl_FragData[1] = vec4(col*(gl_Color.a*1.5), gl_Color.a);
	gl_FragData[2] = vec4(normalize(vary_normal)*0.5+0.5, 0.0);
}
