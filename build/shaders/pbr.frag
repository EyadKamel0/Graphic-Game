#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

// PBR Textures
uniform sampler2D albedoMap;      // Diffuse/Color texture
uniform sampler2D metallicMap;    // Metallic texture
uniform sampler2D roughnessMap;   // Roughness texture
uniform sampler2D normalMap;      // Normal map (optional)

// Flags for which textures are available
uniform int hasAlbedo;
uniform int hasMetallic;
uniform int hasRoughness;
uniform int hasNormalMap;

// Material properties (fallbacks if no texture)
uniform vec3 albedoColor;
uniform float metallicValue;
uniform float roughnessValue;

// Lighting
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

const float PI = 3.14159265359;

// PBR Functions
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    // Sample textures or use fallback values
    vec3 albedo;
    if (hasAlbedo == 1) {
        albedo = pow(texture(albedoMap, TexCoord).rgb, vec3(2.2)); // sRGB to linear
    } else {
        albedo = albedoColor;
    }
    
    float metallic;
    if (hasMetallic == 1) {
        metallic = texture(metallicMap, TexCoord).r;
    } else {
        metallic = metallicValue;
    }
    
    float roughness;
    if (hasRoughness == 1) {
        roughness = texture(roughnessMap, TexCoord).r;
    } else {
        roughness = roughnessValue;
    }
    
    // Ensure roughness is not 0 to avoid division issues
    roughness = max(roughness, 0.05);
    
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    
    // Calculate reflectance at normal incidence (F0)
    // Dielectrics use 0.04, metals use albedo color
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Reflectance equation
    vec3 Lo = vec3(0.0);
    
    // Single directional light
    vec3 L = normalize(lightPos - FragPos);
    vec3 H = normalize(V + L);
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.001 * distance * distance); // Gentle falloff
    vec3 radiance = lightColor * attenuation * 3.0; // Boost light intensity
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic; // Metals have no diffuse
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    
    // Ambient lighting (simple IBL approximation)
    vec3 ambient = vec3(0.15) * albedo; // Increased ambient for space
    
    // Add rim lighting for better visibility in space
    float rim = 1.0 - max(dot(V, N), 0.0);
    rim = pow(rim, 3.0);
    vec3 rimColor = vec3(0.3, 0.4, 0.5) * rim * 0.5;
    
    vec3 color = ambient + Lo + rimColor;
    
    // HDR tonemapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    FragColor = vec4(color, 1.0);
}
