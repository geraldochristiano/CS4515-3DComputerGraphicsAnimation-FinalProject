#version 410

// Global variables for lighting calculations
uniform vec3 kd;
uniform vec3 viewPos;
uniform vec3 lightPosWorld;
uniform vec3 lightColor;
uniform vec3 ks;
uniform float shininess;
uniform bool useBlinnCorrection;

// Output for on-screen color
layout(location = 0) out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(lightPosWorld - fragPos);
    vec3 V = normalize(viewPos - fragPos);
    vec3 R = reflect(-L, N);
    vec3 H = normalize(L + V);

    float diffuseRatio = max(dot(N,L), 0.0);
    float specularRatio = int(!useBlinnCorrection) * pow(max(dot(V, R), 0.0), shininess) +
                          int(useBlinnCorrection) * pow(max(dot(N, H), 0.0), shininess);

    vec3 finalColor = (diffuseRatio * kd * lightColor) + 
                      (specularRatio * ks * lightColor);

    outColor = vec4(finalColor, 1.0);
}