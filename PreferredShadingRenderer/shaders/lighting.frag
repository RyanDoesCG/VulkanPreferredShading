#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Uniforms
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define MAX_OBJECTS 64
layout (set = 0, binding = 0) uniform UniformBuffer {
    vec4 lightPosition;
    vec4 eyePosition;
    vec4 materials[MAX_OBJECTS];
} uniforms;

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Uniform Inputs
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
layout (input_attachment_index = 0, binding = 1) uniform subpassInput positionTexture;
layout (input_attachment_index = 1, binding = 2) uniform subpassInput normalTexture;
layout (input_attachment_index = 2, binding = 3) uniform subpassInput colorTexture;

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Interpolated Inputs
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
layout (location = 0) in vec4 lightPosition;
layout (location = 1) in vec4 eyePosition;
layout (location = 2) in vec2 uvs;

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Per-Fragment Outputs
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
layout (location = 0) out vec4 result;

// 2D white noise function
float random (vec2 co)
	{ // rand
    return 0.5 + (abs(fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453)) * 0.5); 
    } // rand

void main () 
    { // main

    vec4 worldPosition = subpassLoad(positionTexture);
    vec4 worldNormal   = subpassLoad(normalTexture);
    vec4 albedo        = subpassLoad(colorTexture);
    int id = int(albedo.w);

    vec4 material = uniforms.materials[id];

    vec3 l = normalize(lightPosition - worldPosition).xyz;
    vec3 n = normalize(vec3(worldNormal.xyz) + vec3(
        random(vec2(worldNormal.x)) * material.y,
        random(vec2(worldNormal.y)) * material.y,
        random(vec2(worldNormal.z)) * material.y));

    float d = dot (n, l);

    float diffuse = max(d, 0.0) * material.x;

    float metallic = pow(max(d, 0.0), 256) * material.z;
        if (d <= 1.0) metallic = metallic * (d);
        if (d <  0.0) metallic = 0.0;

    float noise = random(uvs) * material.w;
        if (d <= 1.0) noise = noise * (d);
        if (d <  1.0) noise = 0.0;
    
    result = vec4((albedo.xyz * diffuse) + metallic + noise, 1.0);


    } // main