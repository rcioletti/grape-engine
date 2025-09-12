#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;
layout (location = 3) in vec2 fragTexCoord;

layout (location = 0) out vec4 outColor;

struct PointLight {
    vec4 position;
    vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 ambientLightColor;
    PointLight pointLights[10];
    int numLights;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D textures[];

// Debug modes enum - keep in sync with C++ code
#define DEBUG_MODE_NORMAL 0
#define DEBUG_MODE_SHOW_NORMALS 1
#define DEBUG_MODE_SHOW_UVS 2
#define DEBUG_MODE_SHOW_WORLD_POS 3
#define DEBUG_MODE_SHOW_LIGHT_DISTANCE 4
#define DEBUG_MODE_SHOW_LIGHT_DIRECTION 5
#define DEBUG_MODE_SHOW_DOT_PRODUCT 6
#define DEBUG_MODE_TEXTURE_ONLY 7
#define DEBUG_MODE_LIGHTING_ONLY 8

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    int imgIndex;
    int debugMode;  // Add debug mode to push constants
} push;

void main() {
    // Sample texture with bounds checking
    int textureIndex = max(0, push.imgIndex);
    vec4 texColor = texture(textures[nonuniformEXT(textureIndex)], fragTexCoord);

    // Ensure we have a valid surface normal
    vec3 surfaceNormal = normalize(fragNormalWorld);
    if (length(surfaceNormal) < 0.1) {
        surfaceNormal = vec3(0.0, 1.0, 0.0);
    }

    // Early exit for debug modes that don't need lighting calculations
    if (push.debugMode == DEBUG_MODE_SHOW_NORMALS) {
        outColor = vec4(surfaceNormal * 0.5 + 0.5, 1.0);
        return;
    }

    if (push.debugMode == DEBUG_MODE_SHOW_UVS) {
        outColor = vec4(fragTexCoord, 0.0, 1.0);
        return;
    }

    if (push.debugMode == DEBUG_MODE_SHOW_WORLD_POS) {
        outColor = vec4(abs(fragPosWorld) * 0.1, 1.0);
        return;
    }

    if (push.debugMode == DEBUG_MODE_TEXTURE_ONLY) {
        outColor = texColor;
        return;
    }

    // Calculate lighting for remaining debug modes and normal rendering
    vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 diffuseLight = vec3(0.0);
    vec3 specularLight = vec3(0.0);

    vec3 cameraPosWorld = ubo.invView[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);
    int numLights = max(0, min(ubo.numLights, 10));

    // Process each point light
    for (int i = 0; i < numLights; i++) {
        PointLight light = ubo.pointLights[i];
        vec3 lightPos = light.position.xyz;
        vec3 directionToLight = lightPos - fragPosWorld;
        float lightDistance = length(directionToLight);

        if (lightDistance < 0.01) lightDistance = 0.01;
        directionToLight = normalize(directionToLight);

        float attenuation = 0.1 / (1.0 + lightDistance * lightDistance * 0.01);
        float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0.0);
        vec3 lightIntensity = light.color.xyz * light.color.w * attenuation;

        diffuseLight += lightIntensity * cosAngIncidence;

        if (cosAngIncidence > 0.0) {
            vec3 halfAngle = normalize(directionToLight + viewDirection);
            float specularFactor = pow(max(dot(surfaceNormal, halfAngle), 0.0), 32.0);
            specularLight += lightIntensity * specularFactor;
        }

        // Handle light-specific debug modes (using first light)
        if (i == 0) {
            if (push.debugMode == DEBUG_MODE_SHOW_LIGHT_DISTANCE) {
                float normalizedDist = 1.0 - clamp(lightDistance / 10.0, 0.0, 1.0);
                outColor = vec4(vec3(normalizedDist), 1.0);
                return;
            }

            if (push.debugMode == DEBUG_MODE_SHOW_LIGHT_DIRECTION) {
                outColor = vec4(directionToLight * 0.5 + 0.5, 1.0);
                return;
            }

            if (push.debugMode == DEBUG_MODE_SHOW_DOT_PRODUCT) {
                outColor = vec4(vec3(cosAngIncidence), 1.0);
                return;
            }
        }
    }

    // Final color calculation
    vec3 finalColor = (ambientLight + diffuseLight) * texColor.rgb + specularLight;

    if (push.debugMode == DEBUG_MODE_LIGHTING_ONLY) {
        outColor = vec4(diffuseLight + specularLight, 1.0);
        return;
    }

    // Normal rendering
    outColor = vec4(finalColor, texColor.a);
}