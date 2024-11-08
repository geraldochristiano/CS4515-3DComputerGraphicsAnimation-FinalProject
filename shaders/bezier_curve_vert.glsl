#version 410

uniform mat4 mvpMatrix;
uniform vec3 color;

layout(location = 0) in vec3 position;

out vec3 fragColor;

void main(){
	gl_Position = mvpMatrix * vec4(position,1);
	fragColor = color;
}