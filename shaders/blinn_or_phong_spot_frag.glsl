#version 410

//$define_string

#ifndef MAX_NUM_LIGHTS
    #define MAX_NUM_LIGHTS 4
#endif

// Global variables for lighting calculations
layout(std140) uniform Material
{
    vec3 kd;
	vec3 ks;
	float shininess;
	float transparency;
};

struct Light {
    vec3 lightPos;
    float innerCutoff;
    vec3 lightDir;
    float outerCutoff;
    float linearAttenuationCoeff;
    float quadraticAttenuationCoeff;
    vec3 lightDiffuseColor;
    vec3 lightSpecularColor;
};

uniform Light lights[MAX_NUM_LIGHTS];
uniform int numLights;
uniform vec3 viewPos;
uniform bool useBlinnCorrection = true;

uniform bool hasDiffuseMap;
uniform sampler2D diffuseMap;
uniform bool hasNormalMap;
uniform sampler2D normalMap;

// Output for on-screen color
layout(location = 0) out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPosition; // World-space position
in vec3 fragNormal; // World-space normal
in vec2 fragTexCoord;

void main()
{
    vec3 N = normalize(fragNormal);
    if (hasNormalMap){
        N = normalize(texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0);
    }

    vec3 V = normalize(viewPos - fragPosition);

    vec3 finalColor = vec3(0);
  
    for (int i = 0; i < numLights && i < MAX_NUM_LIGHTS; i++){
        Light lt = lights[i];
        vec3 L = normalize(lt.lightPos - fragPosition);

        float diffuseRatio = max(dot(N,L), 0.0);
        float specularRatio = 0;
        if (useBlinnCorrection){
            vec3 H = normalize(L + V);
            specularRatio = pow(max(dot(N, H), 0.0), shininess);
        } else {
            vec3 R = reflect(-L, N);
            specularRatio = pow(max(dot(V, R), 0.0), shininess);
        }

        // Soft edges / brightness falloff
        float theta = dot(L, normalize(-lt.lightDir));
        float epsilon = lt.innerCutoff - lt.outerCutoff;
        float intensity = clamp((theta - lt.outerCutoff) / epsilon, 0.0, 1.0);

        // Attenuation
        float dist = length(lt.lightPos - fragPosition);
        float attenuation = 1.0 / (1.0 + (lt.linearAttenuationCoeff * dist) + (lt.quadraticAttenuationCoeff * dist * dist));
        finalColor += (diffuseRatio * (hasDiffuseMap ? texture(diffuseMap, fragTexCoord).rgb : kd) * lt.lightDiffuseColor * attenuation * intensity) + 
                (specularRatio * ks * lt.lightSpecularColor * attenuation * intensity);      
    }

    outColor = vec4(finalColor, transparency);
}