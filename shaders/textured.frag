#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform int usesBakedLighting;
uniform vec3 portalColor;      // Orange tint for portal
uniform int isPortal;          // 1 if rendering portal
uniform int isShip;            // 1 if rendering ship (brighten it)
uniform int isSolarPanel;      // 1 if rendering solar panel (darken it)
uniform int isCollectible;     // 1 if rendering collectible (glow effect)
uniform vec3 collectibleGlow;  // Glow color for collectibles
uniform float glowPulse;       // Pulse intensity (0-1)
uniform int isHitFlashing;     // 1 if ship is hit flashing
uniform float hitFlashIntensity; // Intensity of hit flash (0-1)
uniform int isShieldBubble;    // 1 if rendering invincibility shield
uniform float shieldPulse;     // Shield animation pulse (0-1)
uniform int isRapidFireEffect; // 1 if rendering rapid fire effect
uniform float rapidFirePulse;  // Rapid fire animation pulse

void main()
{
    vec4 texColor = texture(texture1, TexCoord);
    
    // Apply hit flash effect to ship - red flash overlay
    if (isHitFlashing == 1 && isShip == 1) {
        vec3 flashColor = vec3(1.0, 0.2, 0.1);  // Bright red flash
        texColor.rgb = mix(texColor.rgb, flashColor, hitFlashIntensity * 0.8);
    }
    
    // Shield bubble rendering - translucent cyan sphere
    if (isShieldBubble == 1) {
        vec3 shieldColor = vec3(0.2, 0.8, 1.0);  // Cyan
        float alpha = 0.3 + 0.2 * shieldPulse;   // Pulsing transparency
        FragColor = vec4(shieldColor * (0.8 + 0.4 * shieldPulse), alpha);
        return;
    }
    
    // Rapid fire effect rendering - orange energy aura
    if (isRapidFireEffect == 1) {
        vec3 rapidColor = vec3(1.0, 0.5, 0.0);  // Orange
        float alpha = 0.2 + 0.15 * rapidFirePulse;
        FragColor = vec4(rapidColor * (0.7 + 0.5 * rapidFirePulse), alpha);
        return;
    }
    
    // Portal rendering - orange emissive glow
    if (isPortal == 1) {
        vec3 baseColor = texColor.rgb;
        // Apply orange tint and emissive glow
        vec3 orangeTint = portalColor * 1.5;  // Bright orange
        vec3 result = baseColor * orangeTint + orangeTint * 0.3;  // Add glow
        FragColor = vec4(result, texColor.a);
        return;
    }
    
    // Collectible rendering - pulsing glow effect
    if (isCollectible == 1) {
        vec3 baseColor = texColor.rgb;
        // Pulse from dark (0.2) to bright (1.5) - dramatic pulsing
        float pulse = 0.2 + 1.3 * glowPulse;  // Goes from 0.2 to 1.5
        vec3 glowEffect = collectibleGlow * pulse;
        vec3 emissive = collectibleGlow * glowPulse * 0.8;  // Emissive glow only when bright
        vec3 result = baseColor * glowEffect + emissive;
        FragColor = vec4(result, texColor.a);
        return;
    }
    
    if (usesBakedLighting == 1) {
        vec3 baseColor = texColor.rgb;
        
        if (isShip == 1) {
            // SHIP ONLY: Brighten significantly
            baseColor = mix(vec3(0.25), baseColor, 0.75);  // Lift blacks
            baseColor = baseColor * 1.5;  // Boost brightness
            float saturation = 1.3;
            float luma = dot(baseColor, vec3(0.299, 0.587, 0.114));
            baseColor = mix(vec3(luma), baseColor, saturation);
            baseColor = clamp(baseColor, 0.0, 1.0);
        } else if (isSolarPanel == 1) {
            // SOLAR PANEL: Darken significantly
            baseColor = baseColor * 0.4;  // Reduce brightness to 40%
        } else {
            // Normal baked lighting (original behavior)
            baseColor = mix(vec3(0.15), baseColor, 0.85);
            float contrast = 1.2;
            baseColor = (baseColor - 0.5) * contrast + 0.5;
            float saturation = 1.2;
            float luma = dot(baseColor, vec3(0.299, 0.587, 0.114));
            baseColor = mix(vec3(luma), baseColor, saturation);
        }
        
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        
        float ambientStrength = 0.9;
        vec3 ambient = ambientStrength * lightColor;
        vec3 result = baseColor * (ambient + 0.3 * diff * lightColor);
        
        FragColor = vec4(result, texColor.a);
    } else {
        // Full Phong lighting for non-baked objects (asteroids, etc.)
        float ambientStrength = 0.3;
        vec3 ambient = ambientStrength * lightColor;
        
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        
        float specularStrength = 0.5;
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * lightColor;
        
        vec3 result = (ambient + diffuse + specular) * texColor.rgb;
        FragColor = vec4(result, texColor.a);
    }
}
