#version 450

uniform vec3 viewPos;
uniform samplerCube skybox;

in vec3 fragNormal;
in vec3 fragPosition;

out vec4 fragColor;

void main()
{             
    vec3 I = normalize(fragPosition - viewPos);
    vec3 R = reflect(I, normalize(fragNormal));
    fragColor = vec4(texture(skybox, R).rgb, 1.0);
}