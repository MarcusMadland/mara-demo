$input v_texcoord0

#include "common.sh"

SAMPLER2D(s_texColor, 0);

void main()
{
	vec4 textureSample = texture2D(s_texColor, v_texcoord0);
	gl_FragColor = textureSample;
}
