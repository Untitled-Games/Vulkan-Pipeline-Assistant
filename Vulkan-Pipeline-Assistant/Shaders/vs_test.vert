#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexcoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inColour;

layout(location = 0) out vec4 outColour;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(push_constant) uniform PushConstants {
	mat4 mvp;
} PC;

void main() {
	gl_Position = PC.mvp * vec4(inPosition, 1.0);
	outColour = inColour;
}
