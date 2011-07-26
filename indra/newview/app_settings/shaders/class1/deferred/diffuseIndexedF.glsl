/** 
 * @file diffuseIndexedF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
varying vec3 vary_normal;

void main() 
{
	vec3 col = gl_Color.rgb * diffuseLookup(gl_TexCoord[0].xy).rgb;

	gl_FragData[0] = vec4(col, 0.0);
	gl_FragData[1] = gl_Color.aaaa; // spec
	//gl_FragData[1] = vec4(vec3(gl_Color.a), gl_Color.a+(1.0-gl_Color.a)*gl_Color.a); // spec - from former class3 - maybe better, but not so well tested
	vec3 nvn = normalize(vary_normal);
	gl_FragData[2] = vec4(nvn.xy * 0.5 + 0.5, nvn.z, 0.0);
}
