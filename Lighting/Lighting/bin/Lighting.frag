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


void main()
{
	//Ambient
	float ambientFactor = 0.1f;
	vec3 ambient = (lightColor1 + lightColor2) * ambientFactor ;

	// Diffuse and specular for light 1
	vec3 normal = normalize(Normal);
	vec3 lightDir1 = normalize(lightPosition1 - FragPos);
	float NDotL1 = max(dot(normal, lightDir1), 0.0f);
	vec3 diffuse1 = lightColor1 * NDotL1;

	float specularFactor = 0.8f;
	float shininess = 100.0f;
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 halfDir1 = normalize(lightDir1 + viewDir);
	float NDotH1 = max(dot(normal, halfDir1), 0.0f);
	vec3 specular1 = lightColor1 * specularFactor * pow(NDotH1, shininess);

	// Diffuse and specular for light 2
	vec3 lightDir2 = normalize(lightPosition2 - FragPos);
	float NDotL2 = max(dot(normal, lightDir2), 0.0f);
	vec3 diffuse2 = lightColor2 * NDotL2;

	vec3 halfDir2 = normalize(lightDir2 + viewDir);
	float NDotH2 = max(dot(normal, halfDir2), 0.0f);
	vec3 specular2 = lightColor2 * specularFactor * pow(NDotH2, shininess);
	
	//add lighting and map the color from texture
	vec3 lighting = ambient + (diffuse1 + diffuse2) + (specular1 + specular2);
	vec4 texel = texture(myTexture,TexCoord);
	frag_color = vec4(lighting, 1.0f) * texel;
}