#version 450

#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Uniforms
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define MAX_OBJECTS 64
layout (set = 0, binding = 0) uniform UniformBuffer {
    mat4 model [MAX_OBJECTS];
    mat4 view;
    mat4 proj;
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

layout (location = 0) out vec2 frag_uvs;

void main () 
    { // main

    gl_Position = uniforms.proj * uniforms.view * uniforms.model[id] * vec4(position, 1.0);
    //gl_Position = vec4((vec2(-1.0, -1.0) + (uvs * 2.0)).xy, 0.0, 1.0);   

   frag_uvs    = uvs;

    } // main