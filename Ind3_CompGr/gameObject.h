#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

unsigned int TextureFromFile(const char *path, const string &directory);

class GameObject 
{
public:
    vector<Texture> textures_loaded; // загруженные текстуры
    vector<Mesh> meshes; // вектор мешей  [ (англ. «mesh») — это минимальная единица отрисовки объекта ]
    string directory; // папка с объектом
    glm::mat4 matr; // матрица преобразования объекта

    GameObject(string const &path)
    {
        loadObject(path);
    }

    // рисуем все меши объекта
    void Draw(GLuint program)
    {
        for (auto mesh : meshes)
            mesh.Draw(program);
    }
    
private:
    // дай бог здоровья автору статьи https://ravesli.com/urok-18-zagruzka-modelej-v-opengl/ за загрузку объектов с помощью мешей

    // загружаем объект с помощью Assimp
    void loadObject(string const &path)
    {
        // чтение файла с помощью Assimp
        Assimp::Importer importer;
        // параметр aiProcess_Triangulate - если модель не состоит полностью из треугольников, то необходимо сначала преобразовать все примитивные формы модели в треугольники
        // параметр aiProcess_FlipUVs - переворачивает во время обработки координаты текстуры на оси y, где это необходимо
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
		
        // проверяем, что переменные сцены и корневого узла сцены не являются нулевыми, 
        // а также с помощью проверки одного из флагов сцены убеждаемся, что возвращаемые данные являются полными
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
		
        // путь к файлу с объектом
        directory = path.substr(0, path.find_last_of('/'));

        // начинаем обрабатывать все узлы сцены
        // передаем первый узел (корневой) рекурсивной функции processNode()
        // т.к. каждый узел (возможно) содержит набор дочерних элементов, то необходимо сначала обработать выбранный узел, 
        // а затем продолжить обработку всех его дочерних элементов итд
        processNode(scene->mRootNode, scene);
    }

    // обработка узлов
    void processNode(aiNode *node, const aiScene *scene)
    {
        // получаем меш-индексы
        for(int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            // получаем отдельный меш сцены
            meshes.push_back(processMesh(mesh, scene));
        }
        // выполняем то же самое для потомков текущего меша
        for(int i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene);
    }

    // перевод объекта aiMesh в меш-объект
    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        vector<Vertex> vertices; // вершины
        vector<int> indices; // грани
        vector<Texture> textures; // текстура

        // по всем вершинам текущего меша
        for(int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vert;
            
			// координаты вершины меша
            vert.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			
            // нормаль вершины меша
            vert.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			
            // текстурные координаты вершины меша
            if(mesh->mTextureCoords[0]) // если меш содержит текстурные координаты		
                vert.textureCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            else
                vert.textureCoord = glm::vec2(0.0f, 0.0f);
			
            // добавляем всё, что получили, в вектор вершин искомого меша
            vertices.push_back(vert);
        }

        // по каждой треугольной грани меша (из-за параметра aiProcess_Triangulate)
        for(int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];		
            // Получаем все индексы граней и сохраняем их в векторе indices
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
		
        // чтобы получить материал меша, нам нужно проиндексировать массив mMaterials сцены
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];  

        // загружаем диффузные текстуры меша 
        vector<Texture> objTexture = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture");
        textures.insert(textures.end(), objTexture.begin(), objTexture.end());


        return Mesh(vertices, textures, indices);
    }

    /// <summary>
    /// https://learnopengl.com/Model
    /// Сравниваем путь загружаемой текстуры со всеми текстурами из вектора textures_loaded
    /// Если есть совпадение, то мы пропускаем часть кода загрузки/генерации текстуры и просто используем найденную текстурную структуру в качестве текстуры меша
    /// </summary>
    /// <param name="mat"></param>
    /// <param name="type"></param>
    /// <param name="typeName"></param>
    /// <returns></returns>
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
			
            // проверяем, не была ли текстура загружена ранее, и если - да, то пропускаем загрузку новой текстуры и переходим к следующей итерации
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true; // текстура с тем же путем к файлу уже загружена, переходим к следующей
                    break;
                }
            }
            // если текстура еще не была загружена, то загружаем её
            if(!skip)
            {  
                Texture texture;
                texture.textureID = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                // сохраняем текстуру в массиве с уже загруженными текстурами
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }
};

//  загружает текстуру (с помощью заголовочного файла stb_image.h) и возвращает её идентификатор.
unsigned int TextureFromFile(const char *path, const string &directory)
{
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}