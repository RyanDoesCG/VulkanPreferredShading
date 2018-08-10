#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Interpolated Inputs
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
layout (location = 0) in vec3 worldPosition;
layout (location = 1) in vec3 worldNormal;
layout (location = 2) in vec3 color;
layout (location = 3) in vec2 uvs;
layout (location = 4) flat in int id;

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Per-Fragment Outputs
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
layout (location = 0) out vec4 positionBuffer;
layout (location = 1) out vec4 normalBuffer;
layout (location = 2) out vec4 colorBuffer;

void main () 
    { // main

    positionBuffer = vec4(worldPosition, 1.0);
    normalBuffer   = vec4(worldNormal, 2.0);
    colorBuffer    = vec4(color, id);

    } // main