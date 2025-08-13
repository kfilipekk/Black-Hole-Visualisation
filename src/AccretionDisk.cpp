#include "AccretionDisk.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdlib>
#include <algorithm>

AccretionDisk::AccretionDisk() : VAO(0), VBO(0), EBO(0), totalParticles(0) {
}

AccretionDisk::~AccretionDisk() {
    cleanup();
}

void AccretionDisk::initialize(float blackHoleMass) {
    //Clear existing data
    vertices.clear();
    indices.clear();
    
    //Set random seed for consistent results
    srand(42);
    
    //Generate all disk components
    generateMainDisk(blackHoleMass);
    generateSpiralArms(blackHoleMass);
    generateJets(blackHoleMass);
    generateTorus(blackHoleMass);
    
    //Calculate total particles
    totalParticles = DISK_PARTICLES + SPIRAL_ARMS * ARM_PARTICLES + 2 * JET_PARTICLES + TORUS_PARTICLES;
    
    //Generate indices for point rendering
    for (int i = 0; i < totalParticles; ++i) {
        indices.push_back(i);
    }
    
    //Setup OpenGL buffers
    setupBuffers();
}

void AccretionDisk::update(float blackHoleMass) {
    //For now, reinitialize with new mass
    //Could be optimized to only update affected particles
    initialize(blackHoleMass);
}

void AccretionDisk::generateMainDisk(float blackHoleMass) {
    const float innerRadius = blackHoleMass * 0.6f; //Just outside event horizon
    const float outerRadius = blackHoleMass * 12.0f; //Extended disk
    const float diskThickness = blackHoleMass * 0.8f; //Vertical extent
    
    //Generate particle-based disk structure
    for (int i = 0; i < DISK_PARTICLES; ++i) {
        //Logarithmic radial distribution (more particles closer to center)
        float randomRadius = ((float)rand() / RAND_MAX);
        float radius = innerRadius * pow(outerRadius / innerRadius, randomRadius);
        
        //Random angle
        float angle = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f;
        
        //Vertical distribution with Gaussian-like profile
        float verticalRandom = ((float)rand() / RAND_MAX) - 0.5f;
        float scaleHeight = diskThickness * pow(radius / innerRadius, 0.125f); //Flared disk
        float y = verticalRandom * scaleHeight * exp(-verticalRandom * verticalRandom * 2.0f);
        
        //Position
        float x = radius * cos(angle);
        float z = radius * sin(angle);
        
        //Add small random perturbations for turbulence
        x += (((float)rand() / RAND_MAX) - 0.5f) * radius * 0.02f;
        y += (((float)rand() / RAND_MAX) - 0.5f) * scaleHeight * 0.1f;
        z += (((float)rand() / RAND_MAX) - 0.5f) * radius * 0.02f;
        
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        
        //Velocity (Keplerian + perturbations)
        float keplerianSpeed = sqrt(blackHoleMass / radius);
        float vx = -keplerianSpeed * sin(angle);
        float vy = (((float)rand() / RAND_MAX) - 0.5f) * keplerianSpeed * 0.1f; //Vertical turbulence
        float vz = keplerianSpeed * cos(angle);
        
        //Add radial inflow velocity
        float inflowSpeed = keplerianSpeed * 0.01f * (innerRadius / radius);
        vx += inflowSpeed * cos(angle);
        vz += inflowSpeed * sin(angle);
        
        vertices.push_back(vx);
        vertices.push_back(vy);
        vertices.push_back(vz);
        
        //Temperature (decreases with radius, T ∝ r^-3/4)
        float temperature = pow(innerRadius / radius, 0.75f);
        temperature *= (0.8f + 0.4f * ((float)rand() / RAND_MAX)); //Add variability
        vertices.push_back(temperature);
        
        //Density (decreases with radius and height)
        float density = pow(innerRadius / radius, 1.5f) * exp(-abs(y) / scaleHeight);
        density *= (0.5f + 1.0f * ((float)rand() / RAND_MAX)); //Add variability
        vertices.push_back(density);
    }
}

void AccretionDisk::generateSpiralArms(float blackHoleMass) {
    const float innerRadius = blackHoleMass * 0.6f;
    const float outerRadius = blackHoleMass * 12.0f;
    const float diskThickness = blackHoleMass * 0.8f;
    
    for (int arm = 0; arm < SPIRAL_ARMS; ++arm) {
        float armPhase = (float)arm / (float)SPIRAL_ARMS * 2.0f * 3.14159f;
        
        for (int i = 0; i < ARM_PARTICLES; ++i) {
            float t = (float)i / (float)ARM_PARTICLES;
            float radius = innerRadius + t * (outerRadius - innerRadius);
            
            //Logarithmic spiral pattern
            float spiralTightness = 0.3f;
            float angle = armPhase + log(radius / innerRadius) / spiralTightness;
            
            //Enhanced density along spiral arms
            float spiralWidth = radius * 0.1f;
            float offsetAngle = angle + (((float)rand() / RAND_MAX) - 0.5f) * spiralWidth / radius;
            
            float x = radius * cos(offsetAngle);
            float z = radius * sin(offsetAngle);
            float y = (((float)rand() / RAND_MAX) - 0.5f) * diskThickness * 0.3f;
            
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            
            //Enhanced velocity in spiral arms
            float keplerianSpeed = sqrt(blackHoleMass / radius) * 1.1f;
            float vx = -keplerianSpeed * sin(offsetAngle);
            float vy = (((float)rand() / RAND_MAX) - 0.5f) * keplerianSpeed * 0.15f;
            float vz = keplerianSpeed * cos(offsetAngle);
            
            vertices.push_back(vx);
            vertices.push_back(vy);
            vertices.push_back(vz);
            
            //Higher temperature in spiral arms
            float temperature = pow(innerRadius / radius, 0.75f) * 1.3f;
            vertices.push_back(temperature);
            
            //Higher density in spiral arms
            float density = pow(innerRadius / radius, 1.5f) * 2.0f;
            vertices.push_back(density);
        }
    }
}

void AccretionDisk::generateJets(float blackHoleMass) {
    const float jetHeight = blackHoleMass * 15.0f;
    const float jetRadius = blackHoleMass * 0.3f;
    
    for (int jet = 0; jet < 2; ++jet) { //Top and bottom jets
        float jetDirection = (jet == 0) ? 1.0f : -1.0f;
        
        for (int i = 0; i < JET_PARTICLES; ++i) {
            float t = (float)i / (float)JET_PARTICLES;
            float y = jetDirection * t * jetHeight;
            
            //Conical expansion of jet
            float jetRadiusAtHeight = jetRadius * (1.0f + t * 2.0f);
            float angle = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f;
            float radialPos = ((float)rand() / RAND_MAX) * jetRadiusAtHeight;
            
            float x = radialPos * cos(angle);
            float z = radialPos * sin(angle);
            
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            
            //High-velocity jet material
            float jetSpeed = sqrt(blackHoleMass) * 3.0f * (1.0f - t * 0.5f);
            float vx = (((float)rand() / RAND_MAX) - 0.5f) * jetSpeed * 0.2f;
            float vy = jetDirection * jetSpeed;
            float vz = (((float)rand() / RAND_MAX) - 0.5f) * jetSpeed * 0.2f;
            
            vertices.push_back(vx);
            vertices.push_back(vy);
            vertices.push_back(vz);
            
            //Extremely hot jet material
            float temperature = 2.0f * (1.0f - t * 0.7f);
            vertices.push_back(temperature);
            
            //Lower density in jets
            float density = 0.1f * (1.0f - t);
            vertices.push_back(density);
        }
    }
}

void AccretionDisk::generateTorus(float blackHoleMass) {
    const float torusRadius = blackHoleMass * 3.0f;
    const float torusThickness = blackHoleMass * 0.8f;
    
    for (int i = 0; i < TORUS_PARTICLES; ++i) {
        float torusAngle = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f;
        float poloidalAngle = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f;
        
        float majorR = torusRadius + torusThickness * cos(poloidalAngle);
        float x = majorR * cos(torusAngle);
        float z = majorR * sin(torusAngle);
        float y = torusThickness * sin(poloidalAngle);
        
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        
        //Slower motion in thick torus
        float speed = sqrt(blackHoleMass / majorR) * 0.8f;
        float vx = -speed * sin(torusAngle);
        float vy = (((float)rand() / RAND_MAX) - 0.5f) * speed * 0.3f;
        float vz = speed * cos(torusAngle);
        
        vertices.push_back(vx);
        vertices.push_back(vy);
        vertices.push_back(vz);
        
        //Moderate temperature in torus
        float temperature = 0.6f * (0.7f + 0.6f * ((float)rand() / RAND_MAX));
        vertices.push_back(temperature);
        
        //High density in torus
        float density = 1.5f * (0.8f + 0.4f * ((float)rand() / RAND_MAX));
        vertices.push_back(density);
    }
}

void AccretionDisk::setupBuffers() {
    //Generate OpenGL objects
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    
    //Position attribute (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    //Velocity attribute (vx, vy, vz)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    //Temperature attribute
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    //Density attribute
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(3);
}

void AccretionDisk::render(GLuint shaderProgram, const glm::mat4& model, const glm::mat4& view, 
                          const glm::mat4& projection, float time, float blackHoleMass) {
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(glGetUniformLocation(shaderProgram, "time"), time);
    glUniform1f(glGetUniformLocation(shaderProgram, "blackHoleMass"), blackHoleMass);
    
    //Enable point size modification in vertex shader
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); //Disable depth writing for transparent particles
    
    glBindVertexArray(VAO);
    glDrawElements(GL_POINTS, indices.size(), GL_UNSIGNED_INT, 0);
    
    glDepthMask(GL_TRUE); //Re-enable depth writing
    glDisable(GL_BLEND);
    glDisable(GL_PROGRAM_POINT_SIZE);
}

void AccretionDisk::cleanup() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO != 0) {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }
}

const char* AccretionDisk::getVertexShaderSource() {
    return R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aVelocity;
    layout (location = 2) in float aTemperature;
    layout (location = 3) in float aDensity;

    out vec3 FragPos;
    out vec3 Velocity;
    out float Temperature;
    out float Density;
    out float DistFromCenter;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform float time;

    void main() {
        vec3 pos = aPos;
        
        //Add orbital motion based on distance from center
        float radius = length(pos.xz);
        float orbitalSpeed = 1.0 / sqrt(max(radius, 0.1)); //Keplerian velocity
        float angle = atan(pos.z, pos.x) + time * orbitalSpeed;
        
        //Apply orbital motion
        pos.x = radius * cos(angle);
        pos.z = radius * sin(angle);
        
        //Add turbulence and accretion flow
        float turbulence = sin(time * 3.0 + radius * 10.0) * 0.02;
        pos.y += turbulence * aTemperature;
        
        //Spiral inward motion
        float spiralFactor = 0.98 + 0.02 * sin(time * 0.5);
        pos.xz *= spiralFactor;
        
        gl_Position = projection * view * model * vec4(pos, 1.0);
        
        //Dynamic point size based on density and distance
        float screenDistance = gl_Position.w;
        float baseSize = 2.0 + aDensity * 3.0 + aTemperature * 2.0;
        gl_PointSize = baseSize * (50.0 / screenDistance);
        gl_PointSize = clamp(gl_PointSize, 1.0, 8.0);
        
        FragPos = pos;
        Velocity = aVelocity;
        Temperature = aTemperature;
        Density = aDensity;
        DistFromCenter = radius;
    }
    )";
}

const char* AccretionDisk::getFragmentShaderSource() {
    return R"(
    #version 330 core
    in vec3 FragPos;
    in vec3 Velocity;
    in float Temperature;
    in float Density;
    in float DistFromCenter;
    out vec4 FragColor;

    uniform float time;
    uniform float blackHoleMass;

    vec3 blackbodyColor(float temp) {
        //Simplified blackbody radiation color
        temp = clamp(temp, 0.0, 1.0);
        
        if (temp < 0.25) {
            return mix(vec3(0.1, 0.0, 0.0), vec3(0.8, 0.1, 0.0), temp * 4.0);
        } else if (temp < 0.5) {
            return mix(vec3(0.8, 0.1, 0.0), vec3(1.0, 0.4, 0.0), (temp - 0.25) * 4.0);
        } else if (temp < 0.75) {
            return mix(vec3(1.0, 0.4, 0.0), vec3(1.0, 0.8, 0.2), (temp - 0.5) * 4.0);
        } else {
            return mix(vec3(1.0, 0.8, 0.2), vec3(0.8, 0.9, 1.0), (temp - 0.75) * 4.0);
        }
    }

    void main() {
        //Calculate physical properties
        float radius = DistFromCenter;
        float eventHorizon = blackHoleMass * 0.5;
        
        //Temperature decreases with distance (T ∝ r^-3/4 for accretion disk)
        float physicalTemp = pow(max(radius / eventHorizon, 1.0), -0.75);
        float combinedTemp = Temperature * physicalTemp;
        
        //Doppler shift effect based on velocity
        float velocityMagnitude = length(Velocity);
        float dopplerShift = 1.0 + velocityMagnitude * 0.1;
        
        //Get base color from blackbody radiation
        vec3 baseColor = blackbodyColor(combinedTemp * dopplerShift);
        
        //Add relativistic beaming effect
        float beamingFactor = 1.0 + velocityMagnitude * 0.3;
        baseColor *= beamingFactor;
        
        //Density affects opacity and brightness
        float opacity = Density * smoothstep(eventHorizon * 3.0, eventHorizon, radius);
        opacity *= smoothstep(blackHoleMass * 8.0, blackHoleMass * 2.0, radius);
        
        //Add turbulence-based flickering
        float flicker = 0.8 + 0.2 * sin(time * 15.0 + FragPos.x * 50.0 + FragPos.z * 30.0);
        baseColor *= flicker;
        
        //Add magnetic field reconnection flares
        float reconnectionFlare = 0.0;
        if (sin(time * 2.0 + radius * 5.0) > 0.95) {
            reconnectionFlare = 0.5 * exp(-(time - floor(time * 2.0) / 2.0) * 10.0);
        }
        baseColor += vec3(reconnectionFlare * 2.0, reconnectionFlare, reconnectionFlare * 0.5);
        
        //Gravitational redshift near black hole
        float redshift = 1.0 / sqrt(1.0 - eventHorizon / max(radius, eventHorizon * 1.1));
        baseColor.r *= redshift;
        baseColor.gb /= sqrt(redshift);
        
        //Final alpha with atmospheric perspective
        float finalAlpha = opacity * 0.6 * clamp(combinedTemp * 2.0, 0.1, 1.0);
        
        FragColor = vec4(baseColor, finalAlpha);
    } 
    )";
}
