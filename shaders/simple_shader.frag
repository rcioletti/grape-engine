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

layout(set = 0, binding = 0) uniform GlobalUbo{
	mat4 projection;
	mat4 view;
	mat4 invView;
	vec4 ambientLightColor;
	PointLight pointLights[10];
  int numLights;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D textures[];

layout(push_constant) uniform Push{
	mat4 modelMatrix;
	mat4 normalMatrix;
	int imgIndex;
} push;

void main(){
	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	vec3 specularLight = vec3(0.0);
	vec3 surfaceNormal = normalize(fragNormalWorld);

	vec3 cameraPosWorld = ubo.invView[3].xyz;
	vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

	for(int i = 0; i < ubo.numLights; i++){
		PointLight light = ubo.pointLights[i];
		vec3 directionToLight = light.position.xyz - fragPosWorld;
		float attenuation = 1.0 / dot(directionToLight, directionToLight);
		directionToLight = normalize(directionToLight);

		float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
		vec3 intensity = light.color.xyz * light.color.w * attenuation;

		diffuseLight += intensity * cosAngIncidence;

		vec3 halfAngle = normalize(directionToLight + viewDirection);
		float blindTerm = dot(surfaceNormal, halfAngle);
		blindTerm = clamp(blindTerm, 0, 1);
		blindTerm = pow(blindTerm, 512.0);
		specularLight += intensity * blindTerm;
	}

	vec3 textured = fragColor * texture(textures[nonuniformEXT(push.imgIndex)], fragTexCoord).rgb;
    outColor = vec4(diffuseLight * textured + specularLight * textured, 1.0);
}