$input v_texcoord0

#include "../common.sh"

SAMPLER2D(s_texAlbedo, 0);

void main()
{
	gl_FragColor = texture2D(s_texAlbedo, v_texcoord0);
	//gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
