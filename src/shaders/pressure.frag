#version 330 core

uniform sampler2D inPressure;
uniform sampler2D inOldPressure;
uniform sampler2D divergences;

uniform float cellSize;
uniform float timeStep;

in vec2 Tex;
out vec4 FragColor;

void main()
{
	// Note: old pressures are stored in the x component, new ones stored in the y component
	float alpha = (texture(inPressure, Tex).x - texture(inOldPressure, Tex).x)
              * (texture(inPressure, Tex).x - texture(inOldPressure, Tex).x);
	float beta = 1.0f/4.0f;
	float b = texture(divergences, Tex).x;

	vec2 L = clamp(Tex + vec2(-cellSize * .5f, 0.f), vec2(0.f, 0.f), vec2(1.f, 1.f));
	vec2 R = clamp(Tex + vec2( cellSize * .5f, 0.f), vec2(0.f, 0.f), vec2(1.f, 1.f));
	vec2 T = clamp(Tex + vec2(0.f, cellSize * .5f), vec2(0.f, 0.f), vec2(1.f, 1.f));
	vec2 B = clamp(Tex + vec2(0.f, -cellSize * .5f), vec2(0.f, 0.f), vec2(1.f, 1.f));

	float self = texture(inPressure, Tex).x;
	float fL = texture(inPressure, L).x;
	float fR = texture(inPressure, R).x;
	float fT = texture(inPressure, T).x;
	float fB = texture(inPressure, B).x;

	float res = (fL + fR + fB + fT - b) * beta;
	FragColor = vec4(res, 0.f, 0.f, 1.f);
}