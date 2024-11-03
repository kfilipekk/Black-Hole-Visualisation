#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>

struct Camera {
    float radius;
    float yaw;
    float pitch;
};

Camera camera = {
    5.0f, //radius
    -90.0f, //yaw
    0.0f  //pitch
};

bool firstMouse = true;
double lastX = 400.0, lastY = 300.0;
bool isDragging = false;

//Black hole parameters
float blackHoleMass = 1.0f; //Relative mass (1.0 = default)
bool needsGridUpdate = true;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            isDragging = true;
            firstMouse = true;
        } else if (action == GLFW_RELEASE) {
            isDragging = false;
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!isDragging) {
        return;
    }

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    camera.yaw += xoffset;
    camera.pitch += yoffset;

    if (camera.pitch > 89.0f)
        camera.pitch = 89.0f;
    if (camera.pitch < -89.0f)
        camera.pitch = -89.0f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_UP || key == GLFW_KEY_EQUAL) {
            blackHoleMass += 0.1f;
            if (blackHoleMass > 5.0f) blackHoleMass = 5.0f;
            needsGridUpdate = true;
        }
        else if (key == GLFW_KEY_DOWN || key == GLFW_KEY_MINUS) {
            blackHoleMass -= 0.1f;
            if (blackHoleMass < 0.1f) blackHoleMass = 0.1f;
            needsGridUpdate = true;
        }
        else if (key == GLFW_KEY_R) {
            blackHoleMass = 1.0f;
            needsGridUpdate = true;
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.radius -= (float)yoffset;
    if (camera.radius < 1.0f)
        camera.radius = 1.0f;
    if (camera.radius > 45.0f)
        camera.radius = 45.0f;
}

void updateGrid(std::vector<glm::vec3>& gridPositions) {
    const int gridWidth = 25;
    const int gridHeight = 25;
    
    gridPositions.clear();
    
    //Generate grid vertices with mass-dependent curvature
    for (int j = 0; j <= gridHeight; ++j) {
        for (int i = 0; i <= gridWidth; ++i) {
            float x = (float)(i - gridWidth/2) * 0.4f;
            float z = (float)(j - gridHeight/2) * 0.4f;
            
            //Create spacetime curvature effect (stronger with higher mass)
            float dist = sqrt(x * x + z * z);
            float y = -2.0f * blackHoleMass * exp(-0.3f * dist * dist / blackHoleMass);
            
            gridPositions.push_back(glm::vec3(x, y, z));
        }
    }
}

void updateWindowTitle(GLFWwindow* window) {
    std::stringstream ss;
    ss << "Black Hole Simulator - Mass: " << std::fixed << std::setprecision(1) << blackHoleMass 
       << "x (Use +/- or Up/Down to adjust, R to reset)";
    glfwSetWindowTitle(window, ss.str().c_str());
}
const char* gridVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;

    uniform mat4 viewProj;

    void main() {
        gl_Position = viewProj * vec4(aPos, 1.0);
    }
)";

const char* gridFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    void main() {
        FragColor = vec4(0.3, 0.7, 1.0, 0.8); //Blue grid lines
    } 
)";

const char* diskVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;
    out float DistFromCenter;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoord = aTexCoord;
        DistFromCenter = length(aPos.xz);
    }
)";

const char* diskFragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoord;
    in float DistFromCenter;
    out vec4 FragColor;

    uniform float time;

    void main() {
        float angle = atan(TexCoord.y, TexCoord.x);
        float spiral = sin(angle * 6.0 - time * 2.0) * 0.5 + 0.5;
        
        //Temperature gradient from center outward
        float temp = 1.0 - smoothstep(0.5, 2.0, DistFromCenter);
        
        vec3 hotColor = vec3(1.0, 0.4, 0.1); //Orange-red
        vec3 coolColor = vec3(0.8, 0.2, 0.0); //Dark red
        
        vec3 color = mix(coolColor, hotColor, temp * spiral);
        
        //Alpha falloff at edges
        float alpha = smoothstep(2.5, 1.0, DistFromCenter) * 0.7;
        
        FragColor = vec4(color, alpha);
    } 
)";

const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;

    out vec3 FragPos;
    out vec3 Normal;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;  
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec3 FragPos;
    in vec3 Normal;  

    uniform vec3 lightPos; 
    uniform vec3 lightColor;
    uniform vec3 objectColor;

    void main() {
        //ambient
        float ambientStrength = 0.1;
        vec3 ambient = ambientStrength * lightColor;
  	
        //diffuse 
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
            
        vec3 result = (ambient + diffuse) * objectColor;
        FragColor = vec4(result, 1.0);
    } 
)";


const char* blackHoleFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    void main() {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } 
)";


int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Black Hole Simulator", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    //Set initial window title
    updateWindowTitle(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    //Compile and link shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLuint blackHoleShaderProgram = glCreateProgram();
    glAttachShader(blackHoleShaderProgram, vertexShader);
    GLuint blackHoleFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(blackHoleFragmentShader, 1, &blackHoleFragmentShaderSource, NULL);
    glCompileShader(blackHoleFragmentShader);
    glAttachShader(blackHoleShaderProgram, blackHoleFragmentShader);
    glLinkProgram(blackHoleShaderProgram);

    glDeleteShader(blackHoleFragmentShader);

    //Create grid shader program
    GLuint gridVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(gridVertexShader, 1, &gridVertexShaderSource, NULL);
    glCompileShader(gridVertexShader);

    GLuint gridFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(gridFragmentShader, 1, &gridFragmentShaderSource, NULL);
    glCompileShader(gridFragmentShader);

    GLuint gridShaderProgram = glCreateProgram();
    glAttachShader(gridShaderProgram, gridVertexShader);
    glAttachShader(gridShaderProgram, gridFragmentShader);
    glLinkProgram(gridShaderProgram);

    //Create disk shader program
    GLuint diskVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(diskVertexShader, 1, &diskVertexShaderSource, NULL);
    glCompileShader(diskVertexShader);

    GLuint diskFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(diskFragmentShader, 1, &diskFragmentShaderSource, NULL);
    glCompileShader(diskFragmentShader);

    GLuint diskShaderProgram = glCreateProgram();
    glAttachShader(diskShaderProgram, diskVertexShader);
    glAttachShader(diskShaderProgram, diskFragmentShader);
    glLinkProgram(diskShaderProgram);

    glDeleteShader(gridVertexShader);
    glDeleteShader(gridFragmentShader);
    glDeleteShader(diskVertexShader);
    glDeleteShader(diskFragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    //Grid generation (spacetime visualization)
    const int gridWidth = 25;
    const int gridHeight = 25;
    std::vector<glm::vec3> gridPositions;
    std::vector<unsigned int> gridIndices;

    //Initial grid generation
    updateGrid(gridPositions);

    //Generate grid line indices
    for (int j = 0; j < gridHeight; ++j) {
        for (int i = 0; i < gridWidth; ++i) {
            int current = j * (gridWidth + 1) + i;
            
            //Horizontal lines
            gridIndices.push_back(current);
            gridIndices.push_back(current + 1);
            
            //Vertical lines
            gridIndices.push_back(current);
            gridIndices.push_back(current + gridWidth + 1);
        }
    }

    GLuint gridVBO, gridVAO, gridEBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glGenBuffers(1, &gridEBO);

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridPositions.size() * sizeof(glm::vec3), &gridPositions[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gridEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, gridIndices.size() * sizeof(unsigned int), &gridIndices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    //Accretion disk generation
    const int diskSegments = 64;
    const int diskRings = 16;
    std::vector<float> diskVertices;
    std::vector<unsigned int> diskIndices;

    //Generate disk vertices
    for (int ring = 0; ring <= diskRings; ++ring) {
        float radius = 0.5f + (float)ring / (float)diskRings * 1.5f; //Inner to outer radius
        for (int seg = 0; seg <= diskSegments; ++seg) {
            float angle = (float)seg / (float)diskSegments * 2.0f * 3.1415926f;
            float x = radius * cos(angle);
            float z = radius * sin(angle);
            float y = 0.02f * sin(radius * 3.0f); //Slight vertical wave
            
            //Position
            diskVertices.push_back(x);
            diskVertices.push_back(y);
            diskVertices.push_back(z);
            
            //Texture coordinates for animation
            diskVertices.push_back(x);
            diskVertices.push_back(z);
        }
    }

    //Generate disk indices
    for (int ring = 0; ring < diskRings; ++ring) {
        for (int seg = 0; seg < diskSegments; ++seg) {
            int current = ring * (diskSegments + 1) + seg;
            int next = current + diskSegments + 1;
            
            //Two triangles per quad
            diskIndices.push_back(current);
            diskIndices.push_back(next);
            diskIndices.push_back(current + 1);
            
            diskIndices.push_back(current + 1);
            diskIndices.push_back(next);
            diskIndices.push_back(next + 1);
        }
    }

    GLuint diskVBO, diskVAO, diskEBO;
    glGenVertexArrays(1, &diskVAO);
    glGenBuffers(1, &diskVBO);
    glGenBuffers(1, &diskEBO);

    glBindVertexArray(diskVAO);
    glBindBuffer(GL_ARRAY_BUFFER, diskVBO);
    glBufferData(GL_ARRAY_BUFFER, diskVertices.size() * sizeof(float), &diskVertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, diskEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, diskIndices.size() * sizeof(unsigned int), &diskIndices[0], GL_STATIC_DRAW);
    
    //Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    //Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //Original surface mesh (now simplified)
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    //Create a simple surface for the original mesh
    const int surfaceWidth = 20;
    const int surfaceHeight = 20;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;

    //Calculate vertex positions for surface
    for (int j = 0; j <= surfaceHeight; ++j) {
        for (int i = 0; i <= surfaceWidth; ++i) {
            float x = (float)i / (float)surfaceWidth * 10.0f - 5.0f;
            float y = (float)j / (float)surfaceHeight * 10.0f - 5.0f;
            
            float dist = sqrt(x * x + y * y);
            float z = -2.0f * exp(-0.5f * dist * dist);
            positions.push_back(glm::vec3(x, y, z));
        }
    }

    //Calculate normals for surface
    for (int j = 0; j <= surfaceHeight; ++j) {
        for (int i = 0; i <= surfaceWidth; ++i) {
            float x = (float)i / (float)surfaceWidth * 10.0f - 5.0f;
            float y = (float)j / (float)surfaceHeight * 10.0f - 5.0f;
            
            float dist = sqrt(x * x + y * y);
            float z = -2.0f * exp(-0.5f * dist * dist);

            float df_dx = 2.0f * 0.5f * x * -z;
            float df_dy = 2.0f * 0.5f * y * -z;

            glm::vec3 normal = glm::normalize(glm::vec3(-df_dx, -df_dy, 1.0f));
            normals.push_back(normal);
        }
    }

    //Combine positions and normals into vertices array
    for (size_t i = 0; i < positions.size(); ++i) {
        vertices.push_back(positions[i].x);
        vertices.push_back(positions[i].y);
        vertices.push_back(positions[i].z);
        vertices.push_back(normals[i].x);
        vertices.push_back(normals[i].y);
        vertices.push_back(normals[i].z);
    }

    for (int j = 0; j < surfaceHeight; ++j) {
        for (int i = 0; i < surfaceWidth; ++i) {
            int row1 = j * (surfaceWidth + 1);
            int row2 = (j + 1) * (surfaceWidth + 1);
            
            //triangle 1
            indices.push_back(row1 + i);
            indices.push_back(row1 + i + 1);
            indices.push_back(row2 + i + 1);

            //triangle 2
            indices.push_back(row1 + i);
            indices.push_back(row2 + i + 1);
            indices.push_back(row2 + i);
        }
    }

    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    //Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    //Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //Get uniform locations
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    GLuint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    GLuint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");

    //Black hole sphere generation
    const int sphereSegments = 50;
    std::vector<float> sphereVertices;
    for (int i = 0; i <= sphereSegments; ++i) {
        for (int j = 0; j <= sphereSegments; ++j) {
            float theta = i * 2.0f * 3.1415926f / sphereSegments;
            float phi = j * 3.1415926f / sphereSegments;
            float x = 0.5f * cos(theta) * sin(phi);
            float y = 0.5f * cos(phi);
            float z = 0.5f * sin(theta) * sin(phi);
            sphereVertices.push_back(x);
            sphereVertices.push_back(y);
            sphereVertices.push_back(z);
            sphereVertices.push_back(0); //dummy normal (CHANGE)
            sphereVertices.push_back(0);
            sphereVertices.push_back(0);
        }
    }
    std::vector<unsigned int> sphereIndices;
    for (int i = 0; i < sphereSegments; ++i) {
        for (int j = 0; j < sphereSegments; ++j) {
            int first = (i * (sphereSegments + 1)) + j;
            int second = first + sphereSegments + 1;
            sphereIndices.push_back(first);
            sphereIndices.push_back(second);
            sphereIndices.push_back(first + 1);
            sphereIndices.push_back(second);
            sphereIndices.push_back(second + 1);
            sphereIndices.push_back(first + 1);
        }
    }

    GLuint sphereVBO, sphereVAO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), &sphereVertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), &sphereIndices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        
        //Update grid if mass changed
        if (needsGridUpdate) {
            updateGrid(gridPositions);
            updateWindowTitle(window);
            
            //Update GPU buffer
            glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
            glBufferData(GL_ARRAY_BUFFER, gridPositions.size() * sizeof(glm::vec3), &gridPositions[0], GL_STATIC_DRAW);
            
            needsGridUpdate = false;
        }
        
        glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Calculate camera position
        glm::vec3 cameraPos;
        cameraPos.x = camera.radius * cos(glm::radians(camera.pitch)) * cos(glm::radians(camera.yaw));
        cameraPos.y = camera.radius * sin(glm::radians(camera.pitch));
        cameraPos.z = camera.radius * cos(glm::radians(camera.pitch)) * sin(glm::radians(camera.yaw));

        //Create transformations
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
        glm::mat4 viewProj = projection * view;

        //Draw spacetime grid
        glUseProgram(gridShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(gridShaderProgram, "viewProj"), 1, GL_FALSE, glm::value_ptr(viewProj));
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(gridVAO);
        glDrawElements(GL_LINES, gridIndices.size(), GL_UNSIGNED_INT, 0);
        glDisable(GL_BLEND);

        //Draw accretion disk with animation
        glUseProgram(diskShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(diskShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(diskShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(diskShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(diskShaderProgram, "time"), currentTime);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(diskVAO);
        glDrawElements(GL_TRIANGLES, diskIndices.size(), GL_UNSIGNED_INT, 0);
        glDisable(GL_BLEND);

        //Draw black hole sphere (scaled by mass)
        glUseProgram(blackHoleShaderProgram);
        glm::mat4 blackHoleModel = glm::scale(glm::mat4(1.0f), glm::vec3(blackHoleMass * 0.5f));
        glUniformMatrix4fv(glGetUniformLocation(blackHoleShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(blackHoleModel));
        glUniformMatrix4fv(glGetUniformLocation(blackHoleShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(blackHoleShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteBuffers(1, &gridEBO);
    glDeleteProgram(gridShaderProgram);

    glDeleteVertexArrays(1, &diskVAO);
    glDeleteBuffers(1, &diskVBO);
    glDeleteBuffers(1, &diskEBO);
    glDeleteProgram(diskShaderProgram);

    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
    glDeleteProgram(blackHoleShaderProgram);

    glfwTerminate();
    return 0;
}