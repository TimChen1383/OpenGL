#version 330 core

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

//We modify the value and pass it out
out vec4 frag_color;

uniform sampler2D myTexture;
uniform vec3 lightColor1;
uniform vec3 lightPosition1;
uniform vec3 lightColor2;
uniform vec3 lightPosition2;
uniform vec3 viewPos;

// Directional light uniforms
uniform vec3 dirLightDirection;
uniform vec3 dirLightColor;

// Spotlight (flashlight) uniforms
uniform vec3 spotLightPos;
uniform vec3 spotLightDir;
uniform float spotLightCutoff;
uniform float spotLightOuterCutoff;
uniform float spotLightRange;
uniform vec3 spotLightColor;


void main()
{
	// Directional light calculation
	vec3 normal = normalize(Normal);
	vec3 dirLightDir = normalize(-dirLightDirection);
	float dirDiffuseStrength = max(dot(normal, dirLightDir), 0.0);
	vec3 dirDiffuse = dirLightColor * dirDiffuseStrength;
	vec3 baseAmbient = vec3(0.08, 0.08, 0.10); // Lower ambient for balanced brightness
	vec4 texel = texture(myTexture, TexCoord);
	
	// Spotlight calculation (always add contribution)
	vec3 lightToFrag = normalize(FragPos - spotLightPos);
	vec3 spotDir = normalize(spotLightDir);
	float theta = dot(spotDir, lightToFrag);
	float distance = length(spotLightPos - FragPos);
	float epsilon = spotLightCutoff - spotLightOuterCutoff;
	float intensity = clamp((theta - spotLightOuterCutoff) / epsilon, 0.0, 1.0);
	float attenuation = 1.0 / (1.0 + 0.35 * distance + 0.44 * distance * distance);
	vec3 fragToLight = normalize(spotLightPos - FragPos);
	vec3 spotDiffuse = spotLightColor * max(dot(normal, fragToLight), 0.0) * intensity * attenuation;
	float specularFactor = 2.0f;
	float shininess = 64.0f;
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-fragToLight, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
	vec3 spotSpecular = spotLightColor * specularFactor * spec * intensity * attenuation;

	// Final lighting: ambient + directional + spotlight
	vec3 lighting = baseAmbient + dirDiffuse + spotDiffuse + spotSpecular;
	frag_color = vec4(lighting, 1.0f) * texel;
}