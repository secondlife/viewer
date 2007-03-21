void applyScatter(inout vec3 color);

uniform sampler2D diffuseMap;
uniform sampler2D bumpMap;   
uniform samplerCube environmentMap; //: TEXUNIT4,   // Environment map texture
uniform sampler2D screenTex;   //   : TEXUNIT5

uniform vec3 lightDir;
uniform vec3 specular;
uniform float lightExp;
uniform vec2 fbScale;
uniform float refScale;

float msin(float x) {
   float k = sin(x)+1.0;
   k *= 0.5;
   k *= k;
   return 2.0 * k;
}

float mcos(float x) {
   float k = cos(x)+1.0;
   k *= 0.5;
   k *= k;
   return 2.0 * k;
}

float waveS(vec2 v, float t, float a, float f, vec2 d, float s, sampler1D sinMap) 
{
   return texture1D(sinMap, (dot(d, v)*f + t*s)*f).r*a;
}

float waveC(vec2 v, float t, float a, float f, vec2 d, float s, sampler1D sinMap) 
{
   return texture1D(sinMap, (dot(d, v)*f + t*s)*f).g*a*2.0-1.0;
}

float magnitude(vec3 vec) {
   return sqrt(dot(vec,vec));
}

vec3 mreflect(vec3 i, vec3 n) {
   return i + n * 2.0 * abs(dot(n,i))+vec3(0.0,0.0,0.5);
}

void main() 
{
   vec2 texCoord = gl_TexCoord[0].xy;   // Texture coordinates
   vec2 littleWave1 = gl_TexCoord[0].zw;
   vec2 littleWave2 = gl_TexCoord[1].xy;
   vec2 bigWave = gl_TexCoord[1].zw;
   vec3 viewVec = gl_TexCoord[2].xyz;
   vec4 refCoord = gl_TexCoord[3];
   vec4 col = gl_Color;
   vec4 color;
   
   //get color from alpha map (alpha denotes water depth), rgb denotes water color
   vec4 wcol = texture2D(diffuseMap, texCoord.xy);
      
   //store texture alpha
   float da = wcol.a;
         
   //modulate by incoming water color
   //wcol.a *= refCoord.w;
   
   //scale wcol.a (water depth) for steep transition
   wcol.a *= wcol.a;
   
   //normalize view vector
   viewVec = normalize(viewVec);
   
   //get bigwave normal
   vec3 wavef = texture2D(bumpMap, bigWave).xyz*2.0;
      
   vec3 view = vec3(viewVec.x, viewVec.y, viewVec.z);
   
   float dx = 1.0-(dot(wavef*2.0-vec3(1.0), view))*da;
   dx *= 0.274;
      
   //get detail normals
   vec3 dcol = texture2D(bumpMap, littleWave1+dx*view.xy).rgb*0.75;
   dcol += texture2D(bumpMap, littleWave2+view.xy*dx*0.1).rgb*1.25;
      
   //interpolate between big waves and little waves (big waves in deep water)
   wavef = wavef*wcol.a + dcol*(1.0-wcol.a);
   
   //crunch normal to range [-1,1]
   wavef -= vec3(1,1,1);
   
   //get base fresnel component
   float df = dot(viewVec,wavef);
   //reposition fresnel to latter half of [0,1]
   df = 1.0-clamp(df,0.0,1.0);

   //set output alpha based on fresnel
   color.a = clamp((df+da)*0.5,0.0,1.0);
      
   //calculate reflection vector
   vec3 ref = reflect(viewVec.xyz, wavef);
   
   //get specular component
   float spec = clamp(dot(lightDir, normalize(ref)),0.0,1.0);
      
   //fudge reflection to be more noisy at good angles
   ref.z = ref.z*ref.z+df*df*0.5;
   
   //get diffuse component
   float diff = clamp((abs(dot(ref, wavef))),0.0,1.0)*0.9;
      
   //fudge diffuse for extra contrast and ambience
   diff *= diff;      
   diff += 0.4;
    
   //set diffuse color contribution
   color.rgb = textureCube(environmentMap, ref).rgb*diff;
   
   //harden specular
   spec = pow(spec, lightExp);
   
   //add specular color contribution
   color.rgb += spec * specular;

   //figure out distortion vector (ripply)   
   vec2 distort = clamp(((refCoord.xy/refCoord.z) * 0.5 + 0.5 + wavef.xy*refScale),0.0,0.99);
   
   //read from framebuffer (offset)
   vec4 fb = texture2D(screenTex, distort*fbScale);
   
   //tint by framebuffer
   color.rgb = color.a*color.rgb + (1.0-color.a)*fb.rgb;
   
   //apply fog
   applyScatter(color.rgb);
   
   color.a = spec*0.5+fb.a;
   
   gl_FragColor = color;
}
