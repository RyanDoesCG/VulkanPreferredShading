/* * * * * * * * * * * * * * * * * * * * * * * *
 *  ModelLoader.hpp
 *  Ryan Needham
 *
 *  2018
 * * * * * * * * * * * * * * * * * * * * * * * */
#ifndef ModelLoader_hpp
#define ModelLoader_hpp

#include "VulkanVertex.hpp"
#include <sstream>
#include <fstream>
#include <vector>
#include <limits>
#include <cassert>

struct ModelLoader
    {
    
    /**
     *  load
     *
     *  path     - location of the obj model on disk
     *  vertices - target vertex array
     *  indicse  - target index array
     *
     *  reads an obj in from disk
     */
    static void load (const char* path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, glm::vec3& centroid, float& bounds)
        { // ModelLoader :: load
        
        // make sure we're working with empty arrays
        vertices.clear();
        indices.clear();

        // create some vectors to hold the characteristics
        // we're expecting to handle. These will be parsed
        // into the mesh's vertex and index vectors.
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> uvs;

        // most values will be represented by their index
        // when reading the faces in.
        std::vector<uint32_t> positionIndices;
        std::vector<uint32_t> normalIndices;
        std::vector<uint32_t> uvIndices;
        
        // state to read the file from disk
        std::ifstream input(path);
        std::string buffer;

        // process the data line by line
        while (std::getline(input, buffer))
            { // loop through the file

            std::istringstream in (buffer);
            std::string value;
            std::string type;
            std::getline(in, type, ' ');

            // Read in the vertex / normal / uv data
            if (type ==  "v")
                { // parse vertex position
                glm::vec3 pos; in >> pos.x >> pos.y >> pos.z;
                positions.push_back(pos);
                } // parse vertex position
            
            else if (type == "vn")
                { // parse vertex normal
                glm::vec3 norm; in >> norm.x >> norm.y >> norm.z;
                normals.push_back(glm::normalize(norm));
                } // parse vertex normal

            else if (type == "vt")
                { // parse texture coordinate
                glm::vec2 tc; in >> tc.x >> tc.y;
                uvs.push_back(tc);
                } // parse texture coordinate

            else if (type == "s")
                { // parse smoothing toggle
            
                } // parse smoothing toggle

            else if (type == "usemtl")
                { // parse material file

                } // parse material file

            else if (type == "f")
                { // parse face data
                for (uint32_t i = 0; i < 3; ++i)
                    { // for each vertex in the face
                    std::getline(in, value, '/'); if (!value.empty()) positionIndices.push_back(stoi(value) - 1);
                    std::getline(in, value, '/'); if (!value.empty()) uvIndices.push_back(stoi(value) - 1);
                    std::getline(in, value, ' '); if (!value.empty()) normalIndices.push_back(stoi(value) - 1);
                    } // for each vertex in the face
                } // parse face data
            } // loop through the file

        // Build mesh vertex positions
        // we use this as an opportuinty to process the mesh and
        // compute a bounding sphere for the user to take advantage
        // of or ignore
        vertices.resize(positions.size());
        
        float minX = std::numeric_limits<float>::max(); uint32_t minXIndex = 0;
        float maxX = std::numeric_limits<float>::min(); uint32_t maxXIndex = 0;
        
        float minY = std::numeric_limits<float>::max(); uint32_t minYIndex = 0;
        float maxY = std::numeric_limits<float>::min(); uint32_t maxYIndex = 0;
        
        float minZ = std::numeric_limits<float>::max(); uint32_t minZIndex = 0;
        float maxZ = std::numeric_limits<float>::min(); uint32_t maxZIndex = 0;
        
        for (uint32_t v = 0; v < positions.size(); ++v)
            { // for each vertex position
            Vertex vert;
            vert.position = positions[v];
            vertices[v] = vert;
            
            if      (positions[v].x < minX) { minX = positions[v].x; minXIndex = v; }
            else if (positions[v].x > maxX) { maxX = positions[v].x; maxXIndex = v; }

            if      (positions[v].y < minY) { minY = positions[v].y; minYIndex = v; }
            else if (positions[v].y > maxY) { maxY = positions[v].y; maxYIndex = v; }

            if      (positions[v].z < minZ) { minZ = positions[v].z; minZIndex = v; }
            else if (positions[v].z > maxZ) { maxZ = positions[v].z; maxZIndex = v; }

            } // for each vertex position
            
        assert(minX == positions[minXIndex].x);
        assert(minY == positions[minYIndex].y);
        assert(minZ == positions[minZIndex].z);
 
        assert(maxX == positions[maxXIndex].x);
        assert(maxY == positions[maxYIndex].y);
        assert(maxZ == positions[maxZIndex].z);
        
        // we compute 3 pairs of vertices with the minimum and maximum
        // value of each dimension, giving us a vector across the extents
        // of the x, y and z axes
        float xSpan = glm::length(positions[maxXIndex] - positions[minXIndex]);
        float ySpan = glm::length(positions[maxYIndex] - positions[minYIndex]);
        float zSpan = glm::length(positions[maxZIndex] - positions[minZIndex]);

        // the larges of these spans will be used as the bounding sphere
        // extent, and the centroid of it as the mesh centroid
        glm::vec3 centre = { 0.0f, 0.0f, 0.0f };
        float radius = 0.0f;
        
        if (xSpan > ySpan && xSpan > zSpan)
            {
            centre = positions[minXIndex] + ((positions[minXIndex] - positions[maxXIndex]) * 0.5f);
            radius = xSpan;
            }
        else
        if (ySpan > xSpan && ySpan > zSpan)
            {
            centre = positions[minYIndex] + ((positions[minYIndex] - positions[maxYIndex]) * 0.5f);
            radius = ySpan;
            }
        else
        if (zSpan > xSpan && zSpan > ySpan)
            {
            centre = positions[minZIndex] + ((positions[minZIndex] - positions[maxZIndex]) * 0.5f);
            radius = zSpan;
            }
        else
        
        // we then make a single pass through the vertices and expand the
        // sphere whenever we encounter a point outside of it
        for (uint32_t v = 0; v < positions.size(); ++v)
            { // for each position
            
            float distance = glm::length(positions[v] - centre);
            if (distance > radius)
                radius += distance - radius;
            
            } // for each position
            
        centroid = centre;
        bounds = radius;

        // Build the render mesh indices
        indices.resize(positionIndices.size());
        for (uint32_t i = 0; i < positionIndices.size(); ++i)
            { // for each index
            indices[i] = positionIndices[i];
            } // for each index

        for (uint32_t n = 0; n < indices.size(); ++n)
            { // for each vertex
            vertices[indices[n]].normal = normals[normalIndices[n]];
            vertices[indices[n]].uvs = uvs[uvIndices[n]];
            } // for each vertex

        input.close();
        
        } // ModelLoader :: load
    };

#endif /* ModelLoader_hpp */
