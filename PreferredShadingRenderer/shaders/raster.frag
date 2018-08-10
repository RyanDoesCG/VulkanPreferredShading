#version 450

#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Uniforms
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
layout (set = 0, binding = 1) uniform sampler2D lighting;

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Inputs
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
layout (location = 0) in vec2 frag_uvs;

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Outputs
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
layout (location = 0) out vec4 outColour;

void main () 
    { // main

    outColour = texture (lighting, vec2(frag_uvs.s, frag_uvs.t));

    } // main