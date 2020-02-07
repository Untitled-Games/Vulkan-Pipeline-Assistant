#version 450

struct NestedStruct {
	vec4 vecFour;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexcoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inColour;

layout(location = 0) out vec2 outTexcoord;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(set = 0, binding = 0) uniform MVPBuffer {
	mat4 mvp;
	vec2 arr[2];
	NestedStruct arr2[2];
};

void main() {
	gl_Position = mvp * vec4(inPosition, 1.0);
	outTexcoord = inTexcoord;
}
