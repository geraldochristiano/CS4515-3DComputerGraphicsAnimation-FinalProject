#version 410

in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main(){
	outColor = vec4(fragColor,1);
}