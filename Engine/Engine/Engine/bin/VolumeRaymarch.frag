#version 330 core

in vec3 fragPos;
in vec3 localPos;

out vec4 FragColor;

uniform sampler3D volumeTexture;
uniform sampler2D shadowMap;

uniform mat4 model;
uniform mat4 invModel;
uniform mat4 lightSpaceMatrix;

uniform vec3 cameraPos;
uniform vec3 lightDir;
uniform vec3 lightColor;

uniform float densityMultiplier;
uniform float absorption;
uniform vec3 smokeColor;
uniform float stepSize;
uniform int maxSteps;
uniform float shadowDensityMultiplier;

// Ray-box intersection
// Returns (tNear, tFar) or (-1, -1) if no intersection
vec2 rayBoxIntersect(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax)
{
    vec3 invDir = 1.0 / rayDir;
    vec3 t0 = (boxMin - rayOrigin) * invDir;
    vec3 t1 = (boxMax - rayOrigin) * invDir;

    vec3 tMin = min(t0, t1);
    vec3 tMax = max(t0, t1);

    float tNear = max(max(tMin.x, tMin.y), tMin.z);
    float tFar = min(min(tMax.x, tMax.y), tMax.z);

    if (tNear > tFar || tFar < 0.0)
        return vec2(-1.0);

    return vec2(max(tNear, 0.0), tFar);
}

// Convert local position [-1, 1] to texture coordinates [0, 1]
vec3 localToUV(vec3 local)
{
    return local * 0.5 + 0.5;
}

// Sample density from volume texture
float sampleDensity(vec3 localPos)
{
    vec3 uv = localToUV(localPos);

    // Check bounds
    if (any(lessThan(uv, vec3(0.0))) || any(greaterThan(uv, vec3(1.0))))
        return 0.0;

    return texture(volumeTexture, uv).r * densityMultiplier;
}

// Calculate shadow using shadow map
float calculateShadow(vec3 worldPos)
{
    vec4 lightSpacePos = lightSpaceMatrix * vec4(worldPos, 1.0);
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Check if outside shadow map
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0)
        return 1.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    // PCF shadow
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - 0.005 > pcfDepth ? 0.0 : 1.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

// Simple light marching for self-shadowing
float lightMarch(vec3 localPos, vec3 localLightDir, int steps)
{
    float shadowAccum = 0.0;
    float stepLen = 2.0 / float(steps); // March through unit cube

    for (int i = 0; i < steps; i++)
    {
        localPos += localLightDir * stepLen;

        // Check bounds
        if (any(lessThan(localPos, vec3(-1.0))) || any(greaterThan(localPos, vec3(1.0))))
            break;

        shadowAccum += sampleDensity(localPos) * stepLen;
    }

    return exp(-shadowAccum * shadowDensityMultiplier);
}

void main()
{
    // Calculate ray direction in world space
    vec3 rayDir = normalize(fragPos - cameraPos);

    // Transform ray to local space for volume sampling
    vec3 localRayOrigin = (invModel * vec4(cameraPos, 1.0)).xyz;
    vec3 localRayDir = normalize((invModel * vec4(rayDir, 0.0)).xyz);

    // Calculate light direction in local space
    vec3 localLightDir = normalize((invModel * vec4(-lightDir, 0.0)).xyz);

    // Ray-box intersection in local space [-1, 1]
    vec2 tRange = rayBoxIntersect(localRayOrigin, localRayDir, vec3(-1.0), vec3(1.0));

    if (tRange.x < 0.0)
    {
        discard;
    }

    // Raymarch through volume
    float t = tRange.x;
    float tMax = tRange.y;

    vec3 accumColor = vec3(0.0);
    float accumAlpha = 0.0;
    float transmittance = 1.0;

    // Ambient light
    vec3 ambientLight = vec3(0.3);

    for (int i = 0; i < maxSteps && t < tMax; i++)
    {
        vec3 localSamplePos = localRayOrigin + localRayDir * t;
        vec3 worldSamplePos = (model * vec4(localSamplePos, 1.0)).xyz;

        float density = sampleDensity(localSamplePos);

        if (density > 0.001)
        {
            // Calculate shadow from shadow map (from other objects)
            float shadow = calculateShadow(worldSamplePos);

            // Self-shadowing through volume (light marching)
            float selfShadow = lightMarch(localSamplePos, localLightDir, 8);

            // Combined lighting
            vec3 lighting = ambientLight + lightColor * shadow * selfShadow;

            // Beer-Lambert absorption
            float sampleTransmittance = exp(-density * stepSize * absorption);

            // Accumulate color and opacity
            vec3 sampleColor = smokeColor * lighting * density;
            accumColor += transmittance * sampleColor * stepSize;

            transmittance *= sampleTransmittance;

            // Early exit if nearly opaque
            if (transmittance < 0.01)
                break;
        }

        t += stepSize;
    }

    // Final alpha is 1 - transmittance
    float alpha = 1.0 - transmittance;

    // Output with premultiplied alpha
    FragColor = vec4(accumColor, alpha);
}
