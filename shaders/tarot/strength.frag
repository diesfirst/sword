#version 450

layout(location = 0) out vec4 outColor;

vec2 resolution = vec2(500, 500);
float PI = 3.14;

void main()
{
	vec3 color = vec3(0.);
	vec2 st = gl_FragCoord.xy/resolution;
	color += step(0.5 + cos(st.y*PI)*.25, st.x);
	outColor = vec4(color, 1.);
}