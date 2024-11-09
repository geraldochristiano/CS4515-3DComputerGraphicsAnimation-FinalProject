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
#ifdef LIGHT_TYPE
    #if (LIGHT_TYPE == POINT_LIGHT)
    vec3 lightPos;
    float linearAttenuationCoeff;
    float quadraticAttenuationCoeff;

    #elif (LIGHT_TYPE == DIRECTIONAL_LIGHT)
    vec3 lightDir;

    #endif
#endif
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
    #ifdef LIGHT_TYPE
        // Point lights calculation
        #if (LIGHT_TYPE == POINT_LIGHT)
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
            // Attenuation
            float dist = length(lt.lightPos - fragPosition);
            float attenuation = 1.0 / (1.0 + (lt.linearAttenuationCoeff * dist) + (lt.quadraticAttenuationCoeff * dist * dist));

            if (hasDiffuseMap) {
                finalColor += (diffuseRatio * texture(diffuseMap, fragTexCoord).rgb * lt.lightDiffuseColor * attenuation) + 
                    (specularRatio * ks * lt.lightSpecularColor * attenuation);
            } else {
                finalColor += (diffuseRatio * kd * lt.lightDiffuseColor * attenuation) + 
                    (specularRatio * ks * lt.lightSpecularColor * attenuation);
            }
        }

        // Directional lights calculation
        #elif (LIGHT_TYPE == DIRECTIONAL_LIGHT)
        for (int i = 0; i < numLights && i < MAX_NUM_LIGHTS;i++){
            Light lt = lights[i];
            vec3 L = normalize(-lt.lightDir);

            float diffuseRatio = max(dot(N,L), 0.0);
            float specularRatio = 0;
            if (useBlinnCorrection){
                vec3 H = normalize(L + V);
                specularRatio = pow(max(dot(N, H), 0.0), shininess);
            } else {
                vec3 R = reflect(-L, N);
                specularRatio = pow(max(dot(V, R), 0.0), shininess);
            }
            
            if (hasDiffuseMap){
                finalColor += (diffuseRatio * texture(diffuseMap, fragTexCoord).rgb * lt.lightDiffuseColor) + 
                    (specularRatio * ks * lt.lightSpecularColor);
            } else {
                finalColor += (diffuseRatio * kd * lt.lightDiffuseColor) + 
                              (specularRatio * ks * lt.lightSpecularColor);
            }
        }

        #endif
    #endif

    outColor = vec4(finalColor, transparency);
}