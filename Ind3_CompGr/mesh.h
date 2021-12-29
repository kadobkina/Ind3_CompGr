#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>

using namespace std;

struct Vertex
{
    glm::vec3 position; // координаты вершины
    glm::vec3 normal; // нормаль вершины
    glm::vec2 textureCoord; // текстурные координаты вершины
};

struct Texture 
{
    unsigned int textureID;
    string type;
    string path;
};

class Mesh 
{
public:
    vector<Vertex> vertices; // вершины меша
    vector<Texture> textures; // текстуры меша
    vector<int> indices; // грани меша
    GLuint VAO; // VAO вершины меша

    // Конструктор
    Mesh(vector<Vertex> vert, vector<Texture> text, vector<int> ind)
    {
        this->vertices = vert;
        this->textures = text;
        this->indices = ind;

        // устанавливаем вершинные буферы и указатели атрибутов
        InitPositionBuffers();
    }

    // рисуем меш
    void Draw(GLuint program)
    {
        // Активируем текстурный блок 0, делать этого не обязательно, по умолчанию
        // и так активирован GL_TEXTURE0, это нужно для использования нескольких текстур
        glActiveTexture(GL_TEXTURE0);

        // в uniform кладётся текстурный индекс текстурного блока(для GL_TEXTURE0 - 0, для GL_TEXTURE1 - 1 и тд)
        glUniform1i(glGetUniformLocation(program, textures[0].type.c_str()), 0);
        // связываем текстуру
        glBindTexture(GL_TEXTURE_2D, textures[0].textureID);

        // Привязываем вао
        glBindVertexArray(VAO);
        // Передаем данные на видеокарту(рисуем)
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:
    // VBO EBO вершины
    GLuint VBO, EBO;

    // инициализируем все буферные объекты
    void InitPositionBuffers()
    {
        // создаем буферные объекты
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // привязываем VAO
        glBindVertexArray(VAO);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(2);

        // загружаем данные в вершинный буфер
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Устанавливаем указатели вершинных атрибутов

        // Атрибут с координатами
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // Атрибут с текстурой
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, textureCoord));

        //Отвязываем VAO
        glBindVertexArray(0);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(2);
    }
};