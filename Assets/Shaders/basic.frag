#version 450

layout(location = 0) in vec3 v_Normal;
layout(location = 1) in vec2 v_Uv;

layout(location = 0) out vec4 f_color;

// TODO: add in a texture sampler

void main()
{
    // define these in world space (same as the vertex normals)
    vec3 lightDirection = vec3(0.0, 1, 0.0);
    vec4 lightColor = vec4(1, 1, 1, 1.0);
    vec4 surfaceColor = vec4(0.0, 1.0, 0.0, 1.0);
    
    // light model
    float lightIntensity = max(dot(lightDirection, normalize(v_Normal)), 0);
    vec4 shadedColor = lightColor * surfaceColor * lightIntensity;

    // use the input vertex color
    f_color = shadedColor;
}
