#version 450

#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

/* * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Uniforms
 * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define MAX_OBJECTS 64
layout (set = 0, binding = 0) uniform UniformBuffer {
    mat4 model [MAX_OBJECTS];
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

layout (location = 0) out vec3 frag_worldPosition;
layout (location = 1) out vec3 frag_worldNormal;
layout (location = 2) out vec3 frag_color;
layout (location = 3) out vec3 frag_uvs;
layout (location = 4) out int  frag_id;

void main () 
    { // main

    // uv coordinates are mapped to screen space so they
    // can be rendered to a texture that operates in
    // screen space. We assume meshes have been approved
    // by the pre-process suite
    //
    // uv space      :  [ 0 ... 1 ]
    // screen space  :  [-1 ... 1 ]
    //
    gl_Position = vec4(
        -1.0 + (uvs.s * 2.0),
        -1.0 + ((uvs.t) * 2.0),
        0.0,
        1.0);

    frag_worldPosition = (uniforms.model[id] * vec4(position, 1.0)).xyz;
    frag_worldNormal   = (uniforms.model[id] * vec4(normal, 0.0)).xyz;;
    frag_color         = color;
    frag_id            = id;
    } // main