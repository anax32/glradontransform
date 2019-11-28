#version 450
layout(location = 0)  uniform sampler2D	A;
layout(location = 0)  out vec4 O;

void main ()
{
  ivec2 coord = ivec2(gl_FragCoord.xy);
  O = texelFetch(A, coord, 0);
}
