#version 330 core

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec4 FragPosDirLightSpace;

out vec4 frag_color;

uniform sampler2D myTexture;
uniform sampler2D shadowMap;
uniform vec3 viewPos;

// Directional light uniforms
uniform vec3 dirLightDirection;
uniform vec3 dirLightColor;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(-dirLightDirection);
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);

    // PCF for soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    if(projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}

void main()
{
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(-dirLightDirection);
    vec4 texel = texture(myTexture, TexCoord);

    // Ambient
    vec3 ambient = vec3(0.15, 0.15, 0.15);

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = dirLightColor * diff;

    // Shadow
    float shadow = ShadowCalculation(FragPosDirLightSpace);

    // Final color (no specular for ground)
    vec3 lighting = ambient + diffuse * (1.0 - shadow);
    frag_color = vec4(lighting, 1.0) * texel;
}
