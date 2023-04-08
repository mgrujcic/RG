#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/random.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <cubes.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

void placeModel(Shader& ourShader, Model& ourModel, float rotationAngle, glm::vec3 rotationDirection, glm::vec3 scalingVec, glm::vec3 translationVec, int index);
void placeModel(Shader& ourShader, Model& ourModel, float rotationAngle, glm::vec3 rotationDirection, glm::vec3 scalingVec, glm::vec3 translationVec);

unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 900;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float cutOff;
    float outerCutOff;

};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_BLEND); discard blending instead
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader pointLightShader("resources/shaders/pointlight.vs", "resources/shaders/pointlight.fs");
    // load models
    // -----------
    Model appleTreeModel("resources/objects/apple_tree/apple_tree.obj");
    appleTreeModel.SetShaderTextureNamePrefix("material.");

    Model grassModel("resources/objects/grass/10450_Rectangular_Grass_Patch_v1_iterations-2.obj");
    grassModel.SetShaderTextureNamePrefix("material.");

    Model oakTreeModel("resources/objects/tree2/Tree.obj");
    oakTreeModel.SetShaderTextureNamePrefix("material.");

    Model hazelnutBushModel("resources/objects/hazelnut_bush/Hazelnut.obj");
    hazelnutBushModel.SetShaderTextureNamePrefix("material.");

    Model flower1Model("resources/objects/flower1/marigold.obj");
    flower1Model.SetShaderTextureNamePrefix("material.");

    Model roseModel("resources/objects/rose/rose.obj");
    roseModel.SetShaderTextureNamePrefix("material.");

    Model tree3Model("resources/objects/tree3/Tree.obj");
    roseModel.SetShaderTextureNamePrefix("material.");

    Model angelModel("resources/objects/Angel/18343_Angel_v1.obj");
    angelModel.SetShaderTextureNamePrefix("material.");

    PointLight pointLight;
    pointLight.position = glm::vec3(0.0f);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.75, 0.2, 0.2);
    pointLight.specular = glm::vec3(1.0, 0.3, 0.3);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.001f;
    pointLight.quadratic = 0.005f;

    DirLight dirLight;
    dirLight.direction = glm::normalize(glm::vec3(0.15, -1, 0.2));
    dirLight.ambient = glm::vec3(0.25);
    dirLight.diffuse = glm::vec3(0.4);
    dirLight.specular = glm::vec3(0.4);

    SpotLight spotLight;
    spotLight.position = glm::vec3(0.0f);
    spotLight.direction = glm::vec3(0.0f, -1.0f, 0.0f);
    spotLight.ambient = glm::vec3(0.0f);
    spotLight.diffuse = glm::vec3(0.0f);
    spotLight.specular = glm::vec3(0.0f);
    spotLight.cutOff = glm::cos(glm::radians(15.0f));
    spotLight.outerCutOff = glm::cos(glm::radians(17.2f));
    //skybox setup
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    vector<std::string> faces {
        "resources/textures/skybox/bluecloud_ft.jpg",
        "resources/textures/skybox/bluecloud_bk.jpg",
        "resources/textures/skybox/bluecloud_up.jpg",
        "resources/textures/skybox/bluecloud_dn.jpg",
        "resources/textures/skybox/bluecloud_rt.jpg",
        "resources/textures/skybox/bluecloud_lf.jpg"
    };
    unsigned int skyboxTexture = loadCubemap(faces);

    vector<std::string> facesFOM {
            "resources/textures/skybox/browncloud_ft.jpg",
            "resources/textures/skybox/browncloud_bk.jpg",
            "resources/textures/skybox/browncloud_up.jpg",
            "resources/textures/skybox/browncloud_dn.jpg",
            "resources/textures/skybox/browncloud_rt.jpg",
            "resources/textures/skybox/browncloud_lf.jpg"
    };
    unsigned int skyboxTextureFOM = loadCubemap(facesFOM);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);
    skyboxShader.setInt("skyboxFOM", 1);
    skyboxShader.setFloat("coef", 0.0f);

    /*
    //box
    unsigned int boxVAO, boxVBO;
    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(1, &boxVBO);
    glBindVertexArray(boxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) (3*sizeof(float)) );
    */

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    bool fallOfMan = false;
    float timeOfFall = glfwGetTime();
    float coef = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        // input
        // -----
        processInput(window);
        if(!fallOfMan && programState->camera.Position.x * programState->camera.Position.x + programState->camera.Position.z * programState->camera.Position.z < 25.0f){
            fallOfMan = true;
            timeOfFall = currentFrame;
            cerr << "fall of man\n";
        }

        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();
        auto pointLightPositionSeed = (fallOfMan ? timeOfFall : currentFrame);
        pointLight.position = glm::vec3(7.0f*glm::sin(2*pointLightPositionSeed), 15.0f, 7.0*glm::cos(2*pointLightPositionSeed));

        if(fallOfMan) {

            coef = (currentFrame-timeOfFall < 7.0f ? (currentFrame-timeOfFall)/7.0f : 1.0f);

            spotLight.position = pointLight.position;
            spotLight.direction = glm::normalize(programState->camera.Position - spotLight.position);
            spotLight.diffuse = glm::vec3(1.0f, 0.0f, 0.0f);
            spotLight.specular = glm::vec3(0.4f, 0.0f, 0.0f);

            dirLight.ambient = glm::vec3(0.25) * (1.0f-(5.0f/7.0f)*coef);
            dirLight.diffuse = glm::vec3(0.4) * (1.0f-(5.0f/7.0f)*coef);
            dirLight.specular = glm::vec3(0.4) * (1.0f-(5.0f/7.0f)*coef);

        }
        std::cerr << pointLight.position.x << " " << pointLight.position.y << " " << pointLight.position.z << "\n";

        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);

        ourShader.setVec3("dirLight.direction", dirLight.direction);
        ourShader.setVec3("dirLight.ambient", dirLight.ambient);
        ourShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        ourShader.setVec3("dirLight.specular", dirLight.specular);

        ourShader.setVec3("spotLight.position", spotLight.position);
        ourShader.setVec3("spotLight.direction", spotLight.direction);
        ourShader.setVec3("spotLight.ambient", spotLight.ambient);
        ourShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        ourShader.setVec3("spotLight.specular", spotLight.specular);
        ourShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        ourShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // render appleTreeModel
        placeModel(ourShader, appleTreeModel, 0, glm::vec3(1,0,0), glm::vec3(20), glm::vec3(0, 6.3, -6.5));


        //render tree2
        placeModel(ourShader, oakTreeModel, 0.0f, glm::vec3(0,1,0), glm::vec3(3), glm::vec3(10, 1.5, 15));
        placeModel(ourShader, oakTreeModel, -30.0f, glm::vec3(0,1,0), glm::vec3(3.5), glm::vec3(17, 1.5, -2));
        placeModel(ourShader, oakTreeModel, 30.0f, glm::vec3(0,1,0), glm::vec3(2.5), glm::vec3(20, 1.5, 7));

        //render hazelnut
        placeModel(ourShader, hazelnutBushModel, 0.0f, glm::vec3(0,0,0), glm::vec3(0.7), glm::vec3(-10, 0, -10));


        //render tree3
        placeModel(ourShader, tree3Model, 0, glm::vec3(1.0f), glm::vec3(2.7f), glm::vec3(20, 2, -20));
        placeModel(ourShader, tree3Model, 0, glm::vec3(1.0f), glm::vec3(2.25f), glm::vec3(12, 2, -16));

        //render flower1
        std::vector<glm::vec3> flower1Coordinates = {
                glm::vec3(-5, 1.2, 5),
                glm::vec3(-10, 1.2, 2),
                glm::vec3(-20, 1.2, -3),
                glm::vec3(-5, 1.2, -15),
                glm::vec3(5, 1.2, -12),
                glm::vec3(-12, 1.2, -5),
                glm::vec3(6, 1.2, 5),
                glm::vec3(-5, 1.2, 13)
        };
        for(int i = 0; i < flower1Coordinates.size(); i++) {
            placeModel(ourShader, flower1Model, -90.0f, glm::vec3(1, glm::cos((float) i) * 0.18, 0),
                       glm::vec3(0.06 + 0.015 * glm::sin(i)), flower1Coordinates[i], i);
            placeModel(ourShader, flower1Model, -90.0f, glm::vec3(1, glm::cos((float) i) * 0.18, 0),
                       glm::vec3(0.06 + 0.015 * glm::sin(i)), glm::vec3 (1.1*flower1Coordinates[i].z, flower1Coordinates[i].y, 1.2*flower1Coordinates[i].x), i);
        }

        //render roses
        std::vector<glm::vec3> roseCoordinates = {
                glm::vec3(-5, 1.2, -5),
                glm::vec3(-10, 1.2, -2),
                glm::vec3(20, 1.2, 3),
                glm::vec3(-5, 1.2, 15),
                glm::vec3(-5, 1.2, 12),
                glm::vec3(12, 1.2, 5),
                glm::vec3(6, 1.2, -5),
                glm::vec3(5, 1.2, -13),
                glm::vec3(15, 1.2, -18)
        };
        for(int i = 0; i < roseCoordinates.size(); i++){
            placeModel(ourShader, roseModel, 0, glm::vec3(1, glm::cos((float)i)*0.18,0),
                       glm::vec3(0.03 + 0.008 * glm::sin(i)), roseCoordinates[i], i);
            placeModel(ourShader, roseModel, 0, glm::vec3(1, glm::cos((float)i)*0.18,0),
                       glm::vec3(0.03 + 0.008 * glm::sin(i)), glm::vec3 (1.1*roseCoordinates[i].z, roseCoordinates[i].y, 1.2*roseCoordinates[i].x), i);
        }


        //objects that are face culled
        glEnable(GL_CULL_FACE);

        //render grassModel
        placeModel(ourShader, grassModel, -90.0f, glm::vec3(1,0,0), glm::vec3(0.2), glm::vec3(0));

        glDisable(GL_CULL_FACE);

        //point light source
        pointLightShader.use();
        pointLightShader.setMat4("projection", projection);
        pointLightShader.setMat4("view", view);

        glm::mat4 modelMatrix = glm::mat4(1.0);
        modelMatrix = glm::translate(modelMatrix, pointLight.position);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.3));
        modelMatrix = glm::rotate(modelMatrix, 4*currentFrame + (fallOfMan ? 10 * (currentFrame-timeOfFall) : 0.0f), glm::vec3(0, 1, 0));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(-90.f), glm::vec3(1, 0, 0));

        pointLightShader.setMat4("model", modelMatrix);
        angelModel.Draw(pointLightShader);
    /*  source of light is a box, angel instead

        glm::mat4 modelMatrix = glm::mat4(1.0);
        modelMatrix = glm::translate(modelMatrix, pointLight.position);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.3));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(10.0f), glm::vec3((float)glm::linearRand(0.45, 0.55)));

        pointLightShader.setMat4("model", modelMatrix);
        glBindVertexArray(boxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    */
        //skybox
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);


        skyboxShader.setFloat("coef", coef);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureFOM);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);


        if (programState->ImGuiEnabled)
            DrawImGui(programState);


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    int movementKeysPresed = 0;// fix the faster stepping
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        movementKeysPresed++;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        movementKeysPresed++;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        movementKeysPresed++;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        movementKeysPresed++;



    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime, movementKeysPresed);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime, movementKeysPresed);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime, movementKeysPresed);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime, movementKeysPresed);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

void placeModel(Shader& ourShader, Model& ourModel, float rotationAngle, glm::vec3 rotationDirection, glm::vec3 scalingVec, glm::vec3 translationVec, int index) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, translationVec);
    modelMatrix = glm::scale(modelMatrix, scalingVec);
    if(index != -1)
        modelMatrix = glm::rotate(modelMatrix, glm::radians(index*14.22f) , glm::vec3(0, 1, 0));
    if(rotationAngle != 0.0)
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle) , rotationDirection);

    ourShader.setMat4("model", modelMatrix);
    ourModel.Draw(ourShader);
}


void placeModel(Shader& ourShader, Model& ourModel, float rotationAngle, glm::vec3 rotationDirection, glm::vec3 scalingVec, glm::vec3 translationVec) {
    placeModel(ourShader, ourModel, rotationAngle, rotationDirection, scalingVec, translationVec, -1);
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
