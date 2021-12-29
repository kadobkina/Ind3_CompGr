#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h"
#include "gameObject.h"

#include <iostream>
#include "stb_image.h"

// ID шейдерной программы
GLuint Program;

GLuint Unif_posx;
GLuint Unif_posy;
GLuint Unif_posz;

// размер окна
int width = 800, height = 600;

// камера
Camera camera(glm::vec3(0.0f, 20.0f, 30.0f));

// сцена с объектами
std::vector <GameObject> gameObjects;

// источник света
float lpos[4] = { 0.0f,0.0f,0.0f,1.0f };

// Исходный код вершинного шейдера
const char* VertexShaderSource = R"(
    #version 330 core

    layout (location = 0) in vec3 vertCoord;
    layout (location = 1) in vec3 normal;
    layout (location = 2) in vec2 textCoord;

    out vec2 tCoord;
    out vec3 lightp;
    out vec3 vnormal;
    out vec4 vPosition;

    uniform float xpos;
    uniform float ypos;
    uniform float zpos;

    uniform mat4 object;
    uniform mat4 view;
    uniform mat4 proj;

    void main()
    {
      tCoord = textCoord; 

mat3 aff=mat3(
1, 0, 0,
0, cos(1), -sin(1),
0, sin(1), cos(1)
) * mat3(
cos(1), 0, sin(1),
0, 1, 0,
-sin(1), 0, cos(1)
)* mat3(
            cos(1), sin(1),0,
            -sin(1),cos(1) , 0,
            0, 0, 1
        ); 

      vec3 newNormale = mat3(transpose(inverse(aff))) *  normal;
      vec3 lposition=vec3(xpos,ypos,zpos);
      vec3 temp=lposition; 
 
      gl_Position = proj * view * object * vec4(vertCoord, 1.0);

      vPosition=proj * view * object * vec4(vertCoord, 1.0);
      vnormal=newNormale;
      lightp=  temp-vertCoord;
    }
)";

// Исходный код фрагментного шейдера
const char* FragShaderSource = R"(
    #version 330 core

    in vec3 vCoord;  
    in vec3 vnormal;  
    in vec3 lightp;
    in vec2 tCoord;

    out vec4 color;
    const vec4 diffColor = vec4 ( 0.9, 0.9, 0.9, 1.0 );

    uniform sampler2D ourTexture;

    void main()
    {    
       vec3 n2   = normalize ( vnormal );
       vec3 l2   = normalize ( lightp );
       vec4 diff = diffColor * max ( dot ( n2, l2 ), 0.0 );
       color = texture(ourTexture, tCoord) * diff;
    }
)";

//float xpos = 10.0f;
//float ypos = 30.0f;
//float zpos = 1.0f;
float xpos = 11.0f;
float ypos = 14.0f;
float zpos = -2.0f;
void ChangePos(float x, float y, float z)
{
    if (xpos<1.0f || xpos > -0.9f)
        xpos += x;
    if (ypos < 1.0f || ypos > -0.9f)
        ypos += y;
    if (zpos < 1.0f || zpos > -0.9f)
        zpos += z;

}

// Обработка всех событий ввода: запрос GLFW о нажатии/отпускании кнопки мыши в данном кадре и соответствующая обработка данных событий
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.position[0] -= 1.0;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.position[0] += 1.0;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        ChangePos(-1.1f, 0.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        ChangePos(1.1f, 0.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        ChangePos(0.0f, 1.1f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        ChangePos(0.0f, -1.1f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        ChangePos(0.0f, 0.0f, -1.1f);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        ChangePos(0.0f, 0.0f, 1.1f);
}

// Проверка ошибок OpenGL, если есть то вывод в консоль тип ошибки
void checkOpenGLerror() {
    GLenum errCode;
    // Коды ошибок можно смотреть тут
    // https://www.khronos.org/opengl/wiki/OpenGL_Error
    if ((errCode = glGetError()) != GL_NO_ERROR)
        std::cout << "OpenGl error !: " << errCode << std::endl;
}

// Функция печати лога шейдера
void ShaderLog(unsigned int shader)
{
    int infologLen = 0;
    int charsWritten = 0;
    char* infoLog;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
    if (infologLen > 1)
    {
        infoLog = new char[infologLen];
        if (infoLog == NULL)
        {
            std::cout << "ERROR: Could not allocate InfoLog buffer" << std::endl;
            exit(1);
        }
        glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog);
        std::cout << "InfoLog: " << infoLog << "\n\n\n";
        delete[] infoLog;
    }
}

void setMat4(unsigned int program, const std::string& name, const glm::mat4& mat)
{
    glUniformMatrix4fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void InitShader() {
    // Создаем вершинный шейдер
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    // Передаем исходный код
    glShaderSource(vShader, 1, &VertexShaderSource, NULL);
    // Компилируем шейдер
    glCompileShader(vShader);
    ShaderLog(vShader);

    // Создаем фрагментный шейдер
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    // Передаем исходный код
    glShaderSource(fShader, 1, &FragShaderSource, NULL);
    // Компилируем шейдер
    glCompileShader(fShader);
    ShaderLog(fShader);

    // Создаем программу и прикрепляем шейдеры к ней
    Program = glCreateProgram();
    glAttachShader(Program, vShader);
    glAttachShader(Program, fShader);

    // Линкуем шейдерную программу
    glLinkProgram(Program);
    // Проверяем статус сборки
    int link_ok;
    glGetProgramiv(Program, GL_LINK_STATUS, &link_ok);
    if (!link_ok)
    {
        std::cout << "error attach shaders \n";
        return;
    }
    checkOpenGLerror();

    const char* unif_name = "xpos";
    Unif_posx = glGetUniformLocation(Program, unif_name);
    if (Unif_posx == -1)
    {
        std::cout << "could not bind uniform " << unif_name << std::endl;
        return;
    }
    unif_name = "ypos";
    Unif_posy = glGetUniformLocation(Program, unif_name);
    if (Unif_posy == -1)
    {
        std::cout << "could not bind uniform " << unif_name << std::endl;
        return;
    }
    unif_name = "zpos";
    Unif_posz = glGetUniformLocation(Program, unif_name);
    if (Unif_posz == -1)
    {
        std::cout << "could not bind uniform " << unif_name << std::endl;
        return;
    }

    // смещение света
    glUniform1f(Unif_posx, xpos);
    glUniform1f(Unif_posy, ypos);
    glUniform1f(Unif_posz, zpos);

    // После того, как мы связали шейдеры с нашей программой, удаляем их, т.к. они нам больше не нужны
    glDeleteShader(vShader);
    glDeleteShader(fShader);
}

void InitObjects()
{
    // вид камеры
    setMat4(Program, "view", camera.viewMatrix());

    // проекция
    setMat4(Program, "proj", glm::perspective(glm::radians(50.0f), (float)width / (float)height, 0.1f, 100.0f));

    // загрузка объектов
    GameObject car("objects/car.obj");
    GameObject road("objects/road.obj");
    GameObject grass("objects/grass.obj");

    // двигаем и увеличиваем траву
    glm::mat4 objGrass = glm::mat4(1.0f);
    objGrass = glm::translate(objGrass, glm::vec3(20.0f, -31.0f, -40.0f));
    objGrass = glm::scale(objGrass, glm::vec3(3.0f, 1.5f, 1.0f));
    grass.matr = objGrass;

    // опускаем и уменьшаем дорогу
    glm::mat4 objRoad = glm::mat4(1.0f);
    objRoad = glm::translate(objRoad, glm::vec3(0.0f, -1.0f, 0.0f));
    objRoad = glm::scale(objRoad, glm::vec3(0.88f, 1.0f, 1.0f));
    road.matr = objRoad;

    // приближаем и уменьшаем машину
    glm::mat4 objCar = glm::mat4(1.0f);
    objCar = glm::translate(objCar, glm::vec3(0.0f, 0.0f, 10.0f));
    objCar = glm::scale(objCar, glm::vec3(0.8f, 0.6f, 0.7f));
    car.matr = objCar;

    // добавляем полученные модели в объекты сцены
    gameObjects.push_back(car);
    gameObjects.push_back(road);
    gameObjects.push_back(grass);

}

void Init()
{
    InitShader();
    InitObjects();
    // Включаем проверку глубины
    glEnable(GL_DEPTH_TEST);
}

// Освобождение шейдеров и glwf реcурсов
void Release() {
    // Передавая ноль, мы отключаем шейдрную программу
    glUseProgram(0);
    // Удаляем шейдерную программу
    glDeleteProgram(Program);
    // Освобождение всех glwf реcурсов
    glfwTerminate();
}

int main()
{
    // инициализация glfw
    glfwInit();

    // создание окна
    GLFWwindow* window = glfwCreateWindow(width, height, "Car on the road", NULL, NULL);

    // делаем окно текущим для потока
    glfwMakeContextCurrent(window);

    // загружаем указатели на функции opengl
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // пока текущее окно открыто
    while (!glfwWindowShouldClose(window))
    {
        // движения камеры влево-вправо
        processInput(window);

        // рендеринг
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        // устанавливаем шейдерную программу текущей
        glUseProgram(Program);

        // инициализируем всякие штуки
        Init();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // рисуем объекты
        for (auto go : gameObjects)
        {
            setMat4(Program, "object", go.matr);
            go.Draw(Program);
        }

        // обмен содержимым буферов (отслеживание событий ввода/вывода)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // освобождаем шейдеры и glwf ресурсы
    Release();

    return 0;
}