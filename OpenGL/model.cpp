#include <glad/glad.h> 

#include "glm/glm/glm.hpp"
#include <glm/glm/gtc/matrix_transform.hpp>
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"
#include "shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <chrono>

Texture TextureFromFile(const char* path, const string& directory, bool gamma)
{
    Texture texture;
    string filename = string(path);
    filename = directory + '/' + filename;

    int width, height, nrComponents;
    texture.data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    texture.width = width;
    texture.height = height;
    texture.nrComponents = nrComponents;
    return texture;
}
void TextureToOpenGL(Texture& texture)
{
    glGenTextures(1, &texture.id);
    //auto time = std::chrono::steady_clock::now();

    if (texture.data)
    {
        GLenum format;
        if (texture.nrComponents == 1)
            format = GL_RED;
        else if (texture.nrComponents == 3)
            format = GL_RGB;
        else if (texture.nrComponents == 4)
            format = GL_RGBA;
        auto time = std::chrono::steady_clock::now();
        glBindTexture(GL_TEXTURE_2D, texture.id);
        glTexImage2D(GL_TEXTURE_2D, 0, format, texture.width, texture.height, 0, format, GL_UNSIGNED_BYTE, texture.data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "took " << (float)(std::chrono::steady_clock::now() - time).count() / 1000000000 << " seconds" << std::endl;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(texture.data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << texture.path << std::endl;
        stbi_image_free(texture.data);
    }
    
}