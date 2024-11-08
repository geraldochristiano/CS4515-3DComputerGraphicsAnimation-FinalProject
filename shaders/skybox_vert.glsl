#version 410

uniform mat4 viewProjMatrix;

in vec3 position;

out vec3 texCoords;

void main()
{    
    texCoords = vec3(position.x, position.y, position.z);
    vec4 clipSpacePos = viewProjMatrix * vec4(position, 1.0);
    gl_Position = clipSpacePos.xyww;
} 