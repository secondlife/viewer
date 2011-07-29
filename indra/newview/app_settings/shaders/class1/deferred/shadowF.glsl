/** 
 * @file shadowF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


varying vec4 post_pos;

void main() 
{
	gl_FragColor = vec4(1,1,1,1);
	
	gl_FragDepth = max(post_pos.z/post_pos.w*0.5+0.5, 0.0);
}
