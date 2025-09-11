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

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    int imgIndex;
} push;

void main() {
    // Sample texture with bounds checking
    int textureIndex = max(0, push.imgIndex);
    vec4 texColor = texture(textures[nonuniformEXT(textureIndex)], fragTexCoord);

    // Ensure we have a valid surface normal
    vec3 surfaceNormal = normalize(fragNormalWorld);

    // Check if normal is valid (not zero vector)
    if (length(surfaceNormal) < 0.1) {
        surfaceNormal = vec3(0.0, 1.0, 0.0); // Default up vector
    }

    // Initialize lighting components
    vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 diffuseLight = vec3(0.0);
    vec3 specularLight = vec3(0.0);

    // Get camera position and view direction
    vec3 cameraPosWorld = ubo.invView[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

    // Debug: Ensure we have lights to process
    int numLights = max(0, min(ubo.numLights, 10));

    // Process each point light
    for (int i = 0; i < numLights; i++) {
        PointLight light = ubo.pointLights[i];

        // Calculate direction to light
        vec3 lightPos = light.position.xyz;
        vec3 directionToLight = lightPos - fragPosWorld;
        float lightDistance = length(directionToLight);

        // Avoid division by zero and ensure reasonable distance
        if (lightDistance < 0.01) {
            lightDistance = 0.01;
        }

        // Normalize direction to light
        directionToLight = -normalize(directionToLight);

        // Calculate attenuation (quadratic falloff with minimum distance)
        float attenuation = 0.1 / (1.0 + lightDistance * lightDistance * 0.01);

        // Calculate diffuse lighting
        float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0.0);

        // Light intensity
        vec3 lightIntensity = light.color.xyz * light.color.w * attenuation;

        // Add diffuse contribution
        diffuseLight += lightIntensity * cosAngIncidence;

        // Calculate specular lighting (Blinn-Phong)
        if (cosAngIncidence > 0.0) {
            vec3 halfAngle = normalize(directionToLight + viewDirection);
            float specularFactor = max(dot(surfaceNormal, halfAngle), 0.0);

            // Apply shininess
            float shininess = 32.0; // You might want to make this a uniform
            specularFactor = pow(specularFactor, shininess);

            // Add specular contribution
            specularLight += lightIntensity * specularFactor;
        }
    }

    // Combine all lighting components
    vec3 finalColor = (ambientLight + diffuseLight) * texColor.rgb + specularLight;

    // Output final color
    outColor = vec4(finalColor, texColor.a);

    // STEP-BY-STEP DEBUG - Try these one at a time:

    // 1. Check normals (both sides green = normals are correct)
    // outColor = vec4(surfaceNormal * 0.5 + 0.5, 1.0);

    // 2. Check light distance - visualize distance to first light
    // if (numLights > 0) {
    //     float dist = length(ubo.pointLights[0].position.xyz - fragPosWorld);
    //     outColor = vec4(vec3(1.0 - clamp(dist / 10.0, 0.0, 1.0)), 1.0);
    // }

    // 3. Show light direction vectors (should point towards light)
    // if (numLights > 0) {
    //     vec3 lightDir = normalize(ubo.pointLights[0].position.xyz - fragPosWorld);
    //     outColor = vec4(lightDir * 0.5 + 0.5, 1.0);
    // }

    // 4. Check dot product between normal and light direction
    // if (numLights > 0) {
    //     vec3 lightDir = normalize(ubo.pointLights[0].position.xyz - fragPosWorld);
    //     float dotProduct = dot(surfaceNormal, lightDir);
    //     outColor = vec4(vec3(dotProduct), 1.0);
    // }

    // 5. Show world position to check coordinate system
    //outColor = vec4(abs(fragPosWorld) * 0.1, 1.0);

    // 6. Show light position relative to fragment
    // if (numLights > 0) {
    //     vec3 lightPos = ubo.pointLights[0].position.xyz;
    //     outColor = vec4(abs(lightPos - fragPosWorld) * 0.1, 1.0);
    // }

    // 7. Check if light is actually above the floor
    // if (numLights > 0) {
    //     float lightHeight = ubo.pointLights[0].position.y;
    //     float floorHeight = fragPosWorld.y;
    //     float heightDiff = lightHeight - floorHeight;
    //     outColor = vec4(vec3(heightDiff > 0.0 ? 1.0 : 0.0), 1.0);
    // }
}