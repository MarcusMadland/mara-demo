$input v_texcoord0

#include "common.sh"

SAMPLER2D(s_diffuse, 0);
uniform vec4 u_diffuse;

void main()
{
	vec4 textureSample = texture2D(s_diffuse, v_texcoord0);
	
	if (textureSample.a >= 0.1)
	{
		gl_FragColor = textureSample;
	}
	else
	{
		gl_FragColor = u_diffuse;
	}
}
