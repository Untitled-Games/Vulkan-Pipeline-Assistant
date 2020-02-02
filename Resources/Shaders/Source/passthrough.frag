#version 450

layout(location = 0) in vec2 inTexcoord;

layout(location = 0) out vec4 outColour;

layout(constant_id = 0) const uint attachmentCount = 1;

layout(set = 0, binding = 0) uniform sampler2D attachments[attachmentCount];

layout(push_constant) uniform PushConstants {
	uint attachmentIndex;
} PC;

float LinearizeDepth(float depth, float near, float far) {
	return (2.0 * near) / (far + near - depth * (far - near));
}

void main() {
	if (PC.attachmentIndex == attachmentCount - 1) {
		float linearDepth = LinearizeDepth(texture(attachments[PC.attachmentIndex], inTexcoord).r, 1.0, 100.0);
		outColour = vec4(linearDepth, linearDepth, linearDepth, 1.0);
	}
	else {
		outColour = texture(attachments[PC.attachmentIndex], inTexcoord);
	}
}
