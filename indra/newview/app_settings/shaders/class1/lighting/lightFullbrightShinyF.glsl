/** 
 * @file lightFullbrightShinyF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 



uniform sampler2D diffuseMap;
uniform samplerCube environmentMap;

void fullbright_shiny_lighting() 
{
	gl_FragColor = texture2D(diffuseMap, gl_TexCoord[0].xy);
}
