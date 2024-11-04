#version 410
 
uniform samplerCube skybox;

in vec3 texCoords;

out vec4 fragColor;

void main()
{    
    fragColor = texture(skybox, texCoords);
}