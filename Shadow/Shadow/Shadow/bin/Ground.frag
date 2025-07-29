#version 330 core

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec4 FragPosLightSpace;
in vec4 FragPosDirLightSpace;

//We modify the value and pass it out
out vec4 frag_color;

uniform sampler2D myTexture;
uniform sampler2D shadowMap;
uniform sampler2D dirShadowMap;
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

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // Calculate bias to reduce shadow acne
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(spotLightPos - FragPos);
    float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.001);
    
    // Check whether current frag pos is in shadow
    // PCF (Percentage Closer Filtering) for softer shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

float DirectionalShadowCalculation(vec4 fragPosDirLightSpace)
{
    // Perform perspective divide
    vec3 projCoords = fragPosDirLightSpace.xyz / fragPosDirLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Get closest depth value from directional light's perspective
    float closestDepth = texture(dirShadowMap, projCoords.xy).r; 
    
    // Get depth of current fragment from directional light's perspective
    float currentDepth = projCoords.z;
    
    // Calculate bias to reduce shadow acne for directional light
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(-dirLightDirection);
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    
    // Check whether current frag pos is in shadow
    // PCF (Percentage Closer Filtering) for softer shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(dirShadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(dirShadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

void main()
{
	// Directional light calculation with shadows
	vec3 normal = normalize(Normal);
	vec3 dirLightDir = normalize(-dirLightDirection);
	float dirDiffuseStrength = max(dot(normal, dirLightDir), 0.0);
	float dirShadow = DirectionalShadowCalculation(FragPosDirLightSpace);
	vec3 dirDiffuse = dirLightColor * dirDiffuseStrength * (1.0 - dirShadow);
	vec3 baseAmbient = vec3(0.01, 0.01, 0.01); // Lower ambient for much darker shadows
	vec4 texel = texture(myTexture, TexCoord);
	
	// Calculate spotlight shadow factor
	float shadow = ShadowCalculation(FragPosLightSpace);
	
	// Spotlight calculation with shadow
	vec3 lightToFrag = normalize(FragPos - spotLightPos);
	vec3 spotDir = normalize(spotLightDir);
	float theta = dot(spotDir, lightToFrag);
	float distance = length(spotLightPos - FragPos);
	float epsilon = spotLightCutoff - spotLightOuterCutoff;
	float intensity = clamp((theta - spotLightOuterCutoff) / epsilon, 0.0, 1.0);
	float attenuation = 1.0 / (1.0 + 0.35 * distance + 0.44 * distance * distance);
	vec3 fragToLight = normalize(spotLightPos - FragPos);
	
	// DEBUG: Make shadows SUPER obvious
	// Apply shadow to spotlight diffuse and specular components
	vec3 spotDiffuse = spotLightColor * max(dot(normal, fragToLight), 0.0) * intensity * attenuation * (1.0 - shadow);
	float specularFactor = 2.0f;
	float shininess = 64.0f;
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-fragToLight, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
	vec3 spotSpecular = spotLightColor * specularFactor * spec * intensity * attenuation * (1.0 - shadow);

	// Final lighting: ambient + directional + spotlight with shadows
	vec3 lighting = baseAmbient + dirDiffuse + spotDiffuse + spotSpecular;
	
	frag_color = vec4(lighting, 1.0f) * texel;
}