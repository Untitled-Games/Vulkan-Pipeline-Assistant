#version 450

layout(location = 0) in vec2 inTexcoord;

layout(location = 0) out vec4 outColour;
layout(location = 1) out vec4 outColour2;

layout(push_constant) uniform PushConstants {
	vec4 multiColour;
} PC;

layout(set = 0, binding = 1) uniform sampler2D txTest;

void main()
{
	outColour = texture(txTest, inTexcoord) * PC.multiColour;
	outColour2 = vec4(inTexcoord, 0.0, 1.0);
}
