#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColour;

layout(location = 0) out vec4 outColour;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(push_constant) uniform PushConstants {
	mat4 mvp;
} PC;

void main() {
	gl_Position = vec4(inPosition, 1.0);
	outColour = inColour;
}
