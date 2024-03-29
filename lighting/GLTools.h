#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "HSV.h"
#include "Geometry.h"
#include <algorithm>
#include "LightSource.h"




namespace nr {
	namespace util {
		std::string ReadFile(const std::string& fileName) {
			std::string srcCode;
			std::ifstream vShaderFile;
			vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
			vShaderFile.open(fileName);
			std::stringstream vShaderStream, fShaderStream;
			vShaderStream << vShaderFile.rdbuf();
			vShaderFile.close();
			return vShaderStream.str();
		}
	}
	namespace driver {
		GLFWwindow* window_;
		const unsigned int WINDOWWIDTH = 500;
		const unsigned int WINDOWHEIGHT = 500;
		bool wireframeMode_ = false;
		bool mouseActive_ = true;
		enum class VERTEXATTRIBUTE : GLuint {
			POSITION = 0,
			COLOR = 1,
			NORMAL
		};
		class Camera {
		private:
			float pitch_{ 0 };
			float yaw_{ 90 };
			float roll_{ 0 };

			float eulerSensitivity_ = 0.5;
			const float cameraSpeed_ = 1;
			const float eulerSpeed_ = 4;

			glm::vec2 lastMousePosition_;
			glm::vec2 mousePosition_;

			glm::vec3 cameraPosition_;
			glm::vec3 cameraFront_;
			glm::vec3 cameraUp_;


			void UpdateRotation() {
			
				if (pitch_ > 89.0f)
					pitch_ = 89.0f;
				if (pitch_ < -89.0f)
					pitch_ = -89.0f;

				glm::vec3 direction = glm::vec3(
					cos(glm::radians(yaw_)) * cos(glm::radians(pitch_)),
					sin(glm::radians(pitch_)),
					sin(glm::radians(yaw_)) * cos(glm::radians(pitch_))
				);
				cameraFront_ = glm::normalize(direction);
			}
		public:
			Camera()
				:cameraPosition_(glm::vec3(1.0f, 0.0f, -10.0f)),
				cameraUp_(glm::vec3(0.0f, 1.0f, 0.0f)),
				cameraFront_(glm::vec3(0.0f, 0.0f, -1.0f)),
				mousePosition_(nr::driver::WINDOWWIDTH / 2, nr::driver::WINDOWHEIGHT / 2),
				lastMousePosition_(nr::driver::WINDOWWIDTH / 2, nr::driver::WINDOWHEIGHT / 2){
				UpdateRotation();
			}
			inline void UpdateMousePosition(const glm::vec2& vec) {
				lastMousePosition_ = mousePosition_;
				mousePosition_ = vec;
				glm::vec2 offset = (mousePosition_ - lastMousePosition_) * eulerSensitivity_;

				yaw_ += offset.x;
				pitch_ -= offset.y;
				UpdateRotation();
			}
			inline void MoveNorth() {
				cameraPosition_ = cameraPosition_ + cameraSpeed_ * cameraFront_;
			}
			inline void MoveSouth() {
				cameraPosition_ -= cameraSpeed_ * cameraFront_;
			}
			inline void MoveWest() {
				cameraPosition_ += cameraSpeed_ * glm::normalize(glm::cross(cameraFront_, cameraUp_));
			}
			inline void MoveEast() {
				cameraPosition_ -= cameraSpeed_ * glm::normalize(glm::cross(cameraFront_, cameraUp_));
			}


			inline void LookRight() {
			
				yaw_ += 20;
				UpdateRotation();
			}
			inline void LookLeft() {
				yaw_ -= 20;
				UpdateRotation();
			}
			inline glm::vec3 Position() const noexcept { return cameraPosition_; }
			inline glm::vec3 Up() const noexcept { return cameraUp_; }
			inline glm::vec3 Front() const noexcept { return cameraFront_; }
		};
		class Shader {
		private:
			std::string shaderName_;
			std::string shaderSource_;
			GLuint shaderID_;
		public:
			Shader(const int& shaderType_, const std::string& shaderName, const std::string& shaderSourceFile)
				:
				shaderName_(shaderName),
				shaderSource_(nr::util::ReadFile(shaderSourceFile)) {
				shaderID_ = glCreateShader(shaderType_);
				auto str = shaderSource_.data();
				glShaderSource(shaderID_, 1, &str, NULL);
				glCompileShader(shaderID_);
			}
			bool CheckShader() {
				int success;
				char infoLog[512];
				glGetShaderiv(shaderID_, GL_COMPILE_STATUS, &success);
				if (!success) {
					glGetShaderInfoLog(shaderID_, 512, NULL, infoLog);
					std::cout << infoLog << std::endl;
					return false;
				}
				std::cout << "SHADER FINE" << std::endl;
				return true;
			}
			void Destroy() {
				// dealloc
			}
			inline GLuint ID() const noexcept { return shaderID_; }
			~Shader() {
				Destroy();
			}
		};

		auto camera_ = std::make_unique<nr::driver::Camera>();

		class Program {
		private:
			std::vector<std::unique_ptr<nr::driver::Shader>> shaders_;
			GLuint programID_;
			inline GLuint GetLocation(const std::string& uniformName) const {
				return glGetUniformLocation(programID_, uniformName.data());
			}
		public:
			void RegisterShader(std::unique_ptr<Shader>&& shader) {
				shaders_.push_back(std::move(shader));
			}
			bool Run() {
				for (const auto& shader : shaders_) {
					if (!shader->CheckShader()) return false;
				}
				programID_ = glCreateProgram();
				std::for_each(shaders_.begin(), shaders_.end(), [this](const std::unique_ptr<Shader>& shader) {
					glAttachShader(programID_, shader->ID());
					});
				glLinkProgram(programID_);
				int success;
				char infoLog[512];
				glGetProgramiv(programID_, GL_LINK_STATUS, &success);
				if (!success) {
					glGetProgramInfoLog(programID_, 512, NULL, infoLog);
					std::cout << infoLog << std::endl;
				}
				std::for_each(shaders_.begin(), shaders_.end(), [](std::unique_ptr<Shader>& shader) {
					glDeleteShader(shader->ID());
					});
				return success;
			}
			void Use() {
				glUseProgram(programID_);
			}
			void SetUniformVec3(const std::string& uniformName, const glm::vec3& vec) {
				GLuint uniformLoc = GetLocation(uniformName);
				glUniform3f(uniformLoc, vec.x, vec.y, vec.z);
			}
			void SetUniformMat4(const std::string& uniformName, const glm::mat4& mat) {
				GLuint uniformLoc = GetLocation(uniformName);
				glUniformMatrix4fv(uniformLoc, 1, GL_FALSE, glm::value_ptr(mat));
			}
			void SetUniformInt(const std::string& uniformName, const int& val) {
				GLuint uniformLoc = GetLocation(uniformName);
				glUniform1i(uniformLoc, val);
			}
			void SetUniformFloat(const std::string& uniformName, const float& val) {
				GLuint uniformLoc = GetLocation(uniformName);
				glUniform1f(uniformLoc, val);
			}
		};
	}
}
namespace nr {
	namespace callbacks {
		void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
			switch (key) {
			case GLFW_KEY_SPACE: {
				if (glfwGetKey(window, GLFW_KEY_SPACE) != GLFW_PRESS) break;
				nr::driver::wireframeMode_ = !nr::driver::wireframeMode_;
				if (nr::driver::wireframeMode_)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				break;
			}
			case GLFW_KEY_ESCAPE:{
				if (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) break;
				nr::driver::mouseActive_ = !nr::driver::mouseActive_;

				break;
			}
			case GLFW_KEY_S:{
				nr::driver::camera_->MoveSouth();
				break;
			}
			case GLFW_KEY_W:{
				nr::driver::camera_->MoveNorth();
				break;
			}
			case GLFW_KEY_A:{
				nr::driver::camera_->MoveEast();
				break;
			}
			case GLFW_KEY_D:{
				nr::driver::camera_->MoveWest();
				break;
			}
			case GLFW_KEY_RIGHT:
			{
				nr::driver::camera_->LookRight();
				break;
			}
			}
		}
		void CursorPosCallback(GLFWwindow* window, double xPos, double yPos) {
			if (!nr::driver::mouseActive_) return;
			nr::driver::camera_->UpdateMousePosition({ xPos, yPos });
		}
	}
}
namespace nr {
	namespace driver {
		std::unique_ptr<nr::driver::Program> geometryProgram_;
		std::unique_ptr<nr::driver::Program> lightingProgram_;

		glm::mat4 projectionMatrix_;
		GLuint VAO_;
		GLuint lightVAO_;
		GLuint VBO_;
		GLuint EBO_;
		unsigned int INDEXCOUNT;
		namespace init {
			inline bool InitContext() {
				glfwMakeContextCurrent(nr::driver::window_);
				return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
			}
			bool InitWindow(const unsigned int& width, const unsigned int& height, const char* title) {
				glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
				glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
				nr::driver::window_ = glfwCreateWindow(width, height, title, NULL, NULL);
				return nr::driver::window_;
			}
			void InitCallbacks() {
				glfwSetKeyCallback(nr::driver::window_, &nr::callbacks::KeyCallback);
				glfwSetCursorPosCallback(nr::driver::window_, nr::callbacks::CursorPosCallback);
			}
			void InitShapes(std::vector<float>& vertices, std::vector<unsigned int>& indices) {
				std::array<nr::geometry::Cube, 1> cubes = {
				nr::geometry::Cube (glm::vec3(0.0f, 0.0f, 0.0f), 1.0f),
				};
				for (int i = 0; i < cubes.size(); ++i) {
					nr::geometry::Cube& cube{ cubes.at(i) };
					vertices.insert(vertices.end(), cube.vertices.begin(), cube.vertices.end());
					std::transform(cube.indices.begin(), cube.indices.end(), std::back_inserter(indices), [i](unsigned int index) mutable{
						return index + (i * nr::geometry::Cube::VertexCount());
					});
				}
				nr::driver::INDEXCOUNT = indices.size();
				// specify a normal for a face.
					//
			}
			void InitArrays() {
				std::vector<float> vertices;
				std::vector<unsigned int> indices;

				InitShapes(vertices, indices);
				glGenVertexArrays(1, &VAO_);
				glBindVertexArray(VAO_);

				glGenBuffers(1, &VBO_);
				glGenBuffers(1, &EBO_);

				glBindBuffer(GL_ARRAY_BUFFER, VBO_);
				glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_DYNAMIC_DRAW);

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), indices.data(), GL_STATIC_DRAW);

				glVertexAttribPointer(static_cast<GLuint>(nr::driver::VERTEXATTRIBUTE::POSITION), 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
				//glVertexAttribPointer(static_cast<GLuint>(nr::driver::VERTEXATTRIBUTE::COLOR), 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)(3 * sizeof(float)));
				//glEnableVertexAttribArray(static_cast<GLuint>(nr::driver::VERTEXATTRIBUTE::COLOR));
				glEnableVertexAttribArray(static_cast<GLuint>(nr::driver::VERTEXATTRIBUTE::POSITION));


				// create a new VAO for the lighting, with the same VBO. same data, different interpretation.
				glGenVertexArrays(1, &lightVAO_);
				glBindVertexArray(lightVAO_);

				glBindBuffer(GL_ARRAY_BUFFER, VBO_);
				glVertexAttribPointer(static_cast<GLuint>(nr::driver::VERTEXATTRIBUTE::POSITION), 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
				glEnableVertexAttribArray(static_cast<GLuint>(nr::driver::VERTEXATTRIBUTE::POSITION));


			}
			void InitShaders() {
				geometryProgram_ = std::make_unique<nr::driver::Program>();
				geometryProgram_->RegisterShader(std::make_unique<nr::driver::Shader>(GL_VERTEX_SHADER, "vertexShader", "vertexShader.vert"));
				geometryProgram_->RegisterShader(std::make_unique<nr::driver::Shader>(GL_FRAGMENT_SHADER, "fragmentShader", "fragmentShader.frag"));

				lightingProgram_ = std::make_unique<nr::driver::Program>();
				lightingProgram_->RegisterShader(std::make_unique<nr::driver::Shader>(GL_VERTEX_SHADER, "lightingVertexShader", "vertexShader.vert"));
				lightingProgram_->RegisterShader(std::make_unique<nr::driver::Shader>(GL_FRAGMENT_SHADER, "fragmentShader", "lightSourceFragmentShader.frag"));



			}
			bool InitProgram(const unsigned int& windowWidth, const unsigned int& windowHeight, const char* windowName) {
				if (!glfwInit() || !InitWindow(windowWidth, windowHeight, windowName) || !InitContext()) return false;
				InitCallbacks();
				InitArrays();
				InitShaders();
				geometryProgram_->Run();
				lightingProgram_->Run();
				return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
			}
		}
		void Render() {
			geometryProgram_->Use();
			projectionMatrix_ = glm::mat4(1.0f);
			projectionMatrix_ = glm::perspective(glm::radians(45.0f), (float)nr::driver::WINDOWWIDTH / (float)nr::driver::WINDOWHEIGHT, 0.1f, 10000.0f);
			geometryProgram_->SetUniformMat4("projectionMatrix", projectionMatrix_);
			geometryProgram_->SetUniformFloat("ambientScale", 0.7f);


			nr::lighting::LightSource lightSource_;
			lightSource_.color_ = glm::vec3(1.0f, 1.0f, 1.0f);


			
			int frameNumber = 0;
			while (!glfwWindowShouldClose(window_)) {
				glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);

				glm::mat4 viewMatrix_ = glm::mat4(1.0f);
				viewMatrix_ = glm::lookAt(camera_->Position(), camera_->Position() + camera_->Front(), camera_->Up());
				glm::mat4 modelMatrix_ = glm::mat4(1.0f);

				// props.
				geometryProgram_->Use();
				geometryProgram_->SetUniformMat4("viewMatrix", viewMatrix_);
				geometryProgram_->SetUniformMat4("modelMatrix", modelMatrix_);

				geometryProgram_->SetUniformVec3("objectColor", { 0.2f, 0.7f, 0.0f });

				const float radius{ 50 };
				const float frequency{ 0.0000000001 };
				lightSource_.position_ = glm::vec3(radius * sin(frequency + frameNumber / pow(2, 12)), 0, radius * cos(frequency + frameNumber / pow(2, 12)));
				
				geometryProgram_->SetUniformVec3("lightPosition", lightSource_.position_);
				geometryProgram_->SetUniformVec3("lightColor", lightSource_.color_);
				glBindVertexArray(VAO_);
				glDrawElements(GL_TRIANGLES, nr::driver::INDEXCOUNT , GL_UNSIGNED_INT, (void*)(0 * sizeof(unsigned int)));

				// lighting
				lightingProgram_->Use();
				modelMatrix_ = glm::translate(modelMatrix_, lightSource_.position_);
				lightingProgram_->SetUniformMat4("modelMatrix", modelMatrix_);
				geometryProgram_->SetUniformMat4("viewMatrix", viewMatrix_);
				lightingProgram_->SetUniformMat4("projectionMatrix", projectionMatrix_);

				glDrawElements(GL_TRIANGLES, nr::driver::INDEXCOUNT, GL_UNSIGNED_INT, (void*)(0 * sizeof(unsigned int)));


				glfwPollEvents();
				glfwSwapBuffers(window_);
				++frameNumber;
			}
		}
	}

}