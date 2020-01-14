// See https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
// for reference on this technique. The idea is to only draw one triangle without any vertex or index buffers bound
// and extract a fullscreen quad from it.
#version 450

layout(location = 0) out vec2 outTexcoord;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	outTexcoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(outTexcoord * 2.0 - 1.0, 0.0, 1.0);
}
