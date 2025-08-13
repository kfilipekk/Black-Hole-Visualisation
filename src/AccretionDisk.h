#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

class AccretionDisk {
public:
    AccretionDisk();
    ~AccretionDisk();

    //Initialize the accretion disk with given black hole mass
    void initialize(float blackHoleMass);
    
    //Update the disk (can be used for dynamic changes)
    void update(float blackHoleMass);
    
    //Render the accretion disk
    void render(GLuint shaderProgram, const glm::mat4& model, const glm::mat4& view, 
                const glm::mat4& projection, float time, float blackHoleMass);
    
    //Get shader source code
    static const char* getVertexShaderSource();
    static const char* getFragmentShaderSource();
    
    //Cleanup OpenGL resources
    void cleanup();

private:
    //OpenGL objects
    GLuint VAO, VBO, EBO;
    
    //Disk data
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    int totalParticles;
    
    //Generation parameters
    static constexpr int DISK_PARTICLES = 8192;
    static constexpr int DISK_LAYERS = 32;
    static constexpr int SPIRAL_ARMS = 2;
    static constexpr int ARM_PARTICLES = 1024;
    static constexpr int JET_PARTICLES = 512;
    static constexpr int TORUS_PARTICLES = 2048;
    
    //Private methods for disk generation
    void generateMainDisk(float blackHoleMass);
    void generateSpiralArms(float blackHoleMass);
    void generateJets(float blackHoleMass);
    void generateTorus(float blackHoleMass);
    void setupBuffers();
};
