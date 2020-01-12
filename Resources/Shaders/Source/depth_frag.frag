#version 450

layout(location = 0) in vec2 inTexcoord;

layout(location = 0) out vec4 outColour;

layout(set = 0, binding = 0) uniform sampler2D depthTexture;

float LinearizeDepth(float depth, float near, float far)
{
	return (2.0 * near) / (far + near - depth * (far - near));
}

void main() 
{
	float linearDepth = LinearizeDepth(texture(depthTexture, inTexcoord).r, 1.0, 100.0);
	outColour = vec4(linearDepth, linearDepth, linearDepth, 1.0);
}
