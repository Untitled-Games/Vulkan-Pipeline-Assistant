#version 450

layout(location = 0) in vec4 inColour;

layout(location = 0) out vec4 outColour;

layout(push_constant) uniform PushConstants {
	vec4 colourMask;
} PC;

void main() 
{
	outColour = inColour * PC.colourMask;
}
