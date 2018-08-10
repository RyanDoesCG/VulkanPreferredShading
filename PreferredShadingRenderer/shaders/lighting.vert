#version 450

#extension GL_ARB_separate_shader_objects  : enable
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
 *  Per Vertex Inputs
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 color;
layout (location = 3) in vec2 uvs;
layout (location = 4) in int id;

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  PerVertex Outputs
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
out gl_PerVertex { vec4 gl_Position; };

layout (location = 0) out vec4 lightPosition;
layout (location = 1) out vec4 eyePosition;
layout (location = 2) out vec2 uv;

void main () 
    { // main

    gl_Position = vec4(
        -1.0 + (uvs.s * 2.0),
        -1.0 + ((1.0 - uvs.t) * 2.0),
        0.0,
        1.0);
    
    lightPosition = uniforms.lightPosition;
    eyePosition = uniforms.eyePosition;
    uv = uvs;

    } // main