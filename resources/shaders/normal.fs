#version 330 core
out vec4 FragColor;

struct PointLight {
    vec3 position;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};



struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    sampler2D texture_normal;
    float shininess;
};

struct SpotLight {
    vec3 position;
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float cutOff;
    float outerCutOff;
};



in vec2 texCoords;
in vec3 FragPos;
in mat3 TBN;
in mat3 TBNP;//inverse

vec2 TexCoords;


uniform DirLight dirLight;
uniform SpotLight spotLight;
uniform PointLight pointLight;
uniform Material material;
uniform float height_scale;
uniform bool parallaxMappingToggle;

uniform vec3 viewPosition;
// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords).xxx);
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-dirLight.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    vec3 ambient  = light.ambient  * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));
    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // combine results
    vec3 ambient  = intensity * light.ambient  * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse  = intensity * light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = intensity * light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));

    return (ambient + diffuse + specular);
}

vec2 ParallaxMapping(vec2 texCoords, vec3 lViewDir){
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), lViewDir)));

    float layerDepth = 1.0 / numLayers;

    float currentLayerDepth = 0.0;

    vec2 P = lViewDir.xy / lViewDir.z * height_scale;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    //loaded the displacement map as the specular texture, used for both because it looks similar
    //1 - height because its not an inverse displacement map
    float currentDepthMapValue = 1 - texture(material.texture_specular1, currentTexCoords).r;

    while(currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = 1 - texture(material.texture_specular1, currentTexCoords).r; // 1 - here too
        currentLayerDepth += layerDepth;
    }

    return currentTexCoords;
}

void main()
{
    vec3 viewDir = normalize(viewPosition - FragPos);
    vec3 viewDirTangentSpace = normalize(TBNP * viewDir);
    if(parallaxMappingToggle) {
        TexCoords = ParallaxMapping(texCoords,  viewDirTangentSpace);
    } else {
        TexCoords = texCoords;
    }


    vec3 normal = texture(material.texture_normal, TexCoords).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(TBN * normal);


    vec3 result = CalcDirLight(dirLight, normal, viewDir);
    result += CalcPointLight(pointLight, normal, FragPos, viewDir);
    result += CalcSpotLight(spotLight, normal, FragPos, viewDir);
    vec4 texColor = texture(material.texture_diffuse1, TexCoords);
    if(texColor.a < 0.1)
            discard;
    FragColor = vec4(result, texColor.a);
}