$input v_texcoord0

#include "common.sh"

SAMPLER2D(s_texColor, 0);
uniform vec4 u_color;

void main()
{
	vec4 textureSample = texture2D(s_texColor, v_texcoord0);
	vec4 textureCoord = vec4(v_texcoord0.r, v_texcoord0.g, 0.0, 1.0);
	gl_FragColor = u_color;
}
