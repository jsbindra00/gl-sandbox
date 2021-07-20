#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <random>
#include <fstream>
#include <sstream>
#include "HSV.h"
namespace nr {
	namespace util {
		std::string ReadFile(const std::string& fileName) {
			// 1. retrieve the vertex/fragment source code from filePath
			std::string srcCode;
			std::ifstream vShaderFile;
			// ensure ifstream objects can throw exceptions:
			vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

			// open files
			vShaderFile.open(fileName);
			std::stringstream vShaderStream, fShaderStream;
			// read file's buffer contents into streams
			vShaderStream << vShaderFile.rdbuf();
			// close file handlers
			vShaderFile.close();
			// convert stream into string
			return vShaderStream.str();

		}
	
	}

	namespace driver {
		enum class VERTEXATTRIBUTE : GLuint {
			POSITION = 0,
			COLOR = 1,
			VELOCITY = 2,
			ANGLE = 3
		};
		class Camera {
		private:
			float pitch{ 0 };
			float yaw{ 90 };
			float roll{ 0 };

			glm::vec3 cameraPosition_;

			// ihat for camera
			glm::vec3 cameraFront_;

			// jhat for camera
			glm::vec3 cameraUp_;
			glm::mat4 viewMatrix_;
			const float cameraSpeed_ = 0.5;
			const float eulerSpeed_ = 4;

			void UpdateRotation() {
				glm::vec3 direction;
				direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
				direction.y = sin(glm::radians(pitch));
				direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
				cameraFront_ = glm::normalize(direction);
			}
		public:
			Camera()
				:viewMatrix_(glm::mat4(1.0f)) {
				cameraPosition_ = glm::vec3(2.0f, 3.0f, 2.0f);
				cameraUp_ = glm::vec3(0.0f, 1.0f, 0.0f);
				cameraFront_ = glm::vec3(-0.25f, -0.5f, -1.0f);
			}

			void MoveNorth() {
				// traverse in the direction of the front vector.
				cameraPosition_ = cameraPosition_ + cameraSpeed_ * cameraFront_;
			}
			void MoveSouth() {
				cameraPosition_ -= cameraSpeed_ * cameraFront_;
			}
			void MoveWest() {
				cameraPosition_ += cameraSpeed_ * glm::normalize(glm::cross(cameraFront_, cameraUp_));
			}
			void MoveEast() {
				cameraPosition_ -= cameraSpeed_ * glm::normalize(glm::cross(cameraFront_, cameraUp_));
			}

			// pitch
			void LookUp() {
				pitch += eulerSpeed_;
				UpdateRotation();
			}
			void LookDown() {
				pitch -= eulerSpeed_;
				UpdateRotation();

			}
			// yaw
			void LookLeft() {
				yaw -= eulerSpeed_;
				UpdateRotation();

			}
			void LookRight() {
				yaw += eulerSpeed_;
				UpdateRotation();
			}
			inline glm::vec3 CameraPosition() const noexcept { return cameraPosition_; }
			inline glm::vec3 CameraUp() const noexcept { return cameraUp_; }
			inline glm::vec3 CameraFront() const noexcept { return cameraFront_; }
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
				// allocate an id for the shader
				shaderID_ = glCreateShader(shaderType_);
				// bind the source code
				auto str = shaderSource_.data();
				glShaderSource(shaderID_, 1, &str, NULL);
				// compile shader
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
				shaders_.emplace_back(std::move(shader));
			}
			bool Run() {
				// check if shaders are fine first
				for (const auto& shader : shaders_) {
					if (!shader->CheckShader()) return false;
				}
				// create the program
				programID_ = glCreateProgram();
				// attatch the registered shaders
				std::for_each(shaders_.begin(), shaders_.end(), [this](const std::unique_ptr<Shader>& shader) {
					glAttachShader(programID_, shader->ID());
					});

				// link the program
				glLinkProgram(programID_);
				int success;
				char infoLog[512];
				glGetProgramiv(programID_, GL_LINK_STATUS, &success);
				if (!success) {
					glGetProgramInfoLog(programID_, 512, NULL, infoLog);
					std::cout << infoLog << std::endl;
				}
				// decouple the shaders
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
	namespace util {
		float RADIUS = 0.05;
		class Random {
		private:

			std::random_device rd;
			std::mt19937 randomGenerator;
			std::vector <std::pair<nr::driver::VERTEXATTRIBUTE, std::uniform_real_distribution<float>>> distribs;
		public:
			Random() :
				randomGenerator(rd()) {

				const auto PI = 3.14159265;
				distribs.push_back(std::make_pair< nr::driver::VERTEXATTRIBUTE, std::uniform_real_distribution<float >>(nr::driver::VERTEXATTRIBUTE::POSITION, std::uniform_real_distribution<float >(-RADIUS, RADIUS)));
				distribs.push_back(std::make_pair< nr::driver::VERTEXATTRIBUTE, std::uniform_real_distribution<float >>(nr::driver::VERTEXATTRIBUTE::COLOR, std::uniform_real_distribution<float >(0, 1)));
				distribs.push_back(std::make_pair< nr::driver::VERTEXATTRIBUTE, std::uniform_real_distribution<float >>(nr::driver::VERTEXATTRIBUTE::ANGLE, std::uniform_real_distribution<float >(0, 2 * PI)));
				distribs.push_back(std::make_pair< nr::driver::VERTEXATTRIBUTE, std::uniform_real_distribution<float >>(nr::driver::VERTEXATTRIBUTE::VELOCITY, std::uniform_real_distribution<float >(-0.01, 0.01)));
			}

			float randomNumber(const nr::driver::VERTEXATTRIBUTE& attrib) {
				auto found = std::find_if(distribs.begin(), distribs.end(), [&attrib](const auto& p) {
					return p.first == attrib;
					});
				if (found == distribs.end()) throw "register the attrib u fucking idiot;";
				return found->second(randomGenerator);
			}
		};
		float GetElapsedTime() {
			return glfwGetTime();
		}
		glm::vec3 RandomVector(const float& lowerBound, const float& upperBound) {
			std::random_device rd;
			std::mt19937 randomGenerator(rd());
			std::uniform_real_distribution<float> distrib(lowerBound, upperBound);
			return { distrib(randomGenerator), distrib(randomGenerator), distrib(randomGenerator) };
		}

	}
	namespace callbacks {
		void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
			switch (key) {
			case GLFW_KEY_S:
			{
				nr::driver::camera_->MoveSouth();
				break;
			}
			case GLFW_KEY_W:
			{
				nr::driver::camera_->MoveNorth();
				break;
			}
			case GLFW_KEY_A:
			{
				nr::driver::camera_->MoveEast();
				break;
			}
			case GLFW_KEY_D:
			{
				nr::driver::camera_->MoveWest();
				break;
			}
			case GLFW_KEY_LEFT:
			{
				nr::driver::camera_->LookLeft();
				break;
			}
			case GLFW_KEY_RIGHT:
			{
				nr::driver::camera_->LookRight();
				break;
			}
			case GLFW_KEY_UP: {
				nr::driver::camera_->LookUp();
				break;
			}
			case GLFW_KEY_DOWN: {
				nr::driver::camera_->LookDown();
				break;
			}
			}
		}
	}
}



namespace nr {
	namespace driver {
		std::unique_ptr<nr::driver::Program> shaderProgram_;
		std::unique_ptr<nr::driver::Program> shaderProgram2_;
		GLFWwindow* window_;
		glm::mat4 projectionMatrix_;
		GLuint VAO_;
		GLuint VBO_;
		GLuint EBO_;
		bool wireframeMode_ = true;
		constexpr unsigned int NUM_POINTS = 1000;

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
			inline void InitCallbacks() {
				glfwSetKeyCallback(nr::driver::window_, &nr::callbacks::KeyCallback);


			}
			void InitArrays() {
				// store a single VAO, which stores a single VBO for all particles.
				glGenVertexArrays(1, &VAO_);
				glGenBuffers(1, &VBO_);
				// bind the VAO
				glBindVertexArray(VAO_);

				// generate the particles
				constexpr unsigned int NUM_COMPONENTS_PER_VERTEX = 6;
				float SPAN_X = 2 * 3.14159;

				// each points has 6 attribs
					// x y z r g b

				std::vector<float> vertices;

	
				for (unsigned int i = 0; i < NUM_POINTS; ++i) {


					float x = (i / static_cast<float>(NUM_POINTS)) * SPAN_X;
					vertices.push_back(x); // x
					vertices.push_back(0); // y
					vertices.push_back(0); // z

					
					HSV::RGB col = HSV::HSVtoRGB((i / static_cast<float>(NUM_POINTS)) * 360, 100, 100);
		
					vertices.push_back(col.r / 255);
					vertices.push_back(col.g / 255);
					vertices.push_back(col.b / 255);
				}


				
				// copy the data into the VBO
				// bind the VBO
				glBindBuffer(GL_ARRAY_BUFFER, VBO_);

				// copy the vertex data into the vbo.
				glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_DYNAMIC_DRAW);

				// bind the vertex attrib pointers.

				// bind the position
				glVertexAttribPointer(static_cast<GLuint>(nr::driver::VERTEXATTRIBUTE::POSITION), 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)0);

				// bind the color

				glVertexAttribPointer(static_cast<GLuint>(nr::driver::VERTEXATTRIBUTE::COLOR), 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(3*sizeof(float)));

				// enable the vertex attributes
				glEnableVertexAttribArray(static_cast<GLuint>(nr::driver::VERTEXATTRIBUTE::POSITION));
				glEnableVertexAttribArray(static_cast<GLuint>(nr::driver::VERTEXATTRIBUTE::COLOR));

				std::cout << "done" << std::endl;
			}




			void InitShaders() {
				shaderProgram_ = std::make_unique<nr::driver::Program>();
				shaderProgram2_ = std::make_unique<nr::driver::Program>();

				// vertex shader
				shaderProgram_->RegisterShader(std::make_unique<nr::driver::Shader>(GL_VERTEX_SHADER, "vertexShader", "vertexShader.vert"));

				// vertex shader 2
				
				shaderProgram2_->RegisterShader(std::make_unique<nr::driver::Shader>(GL_VERTEX_SHADER, "vertexShaderz", "vertexShaderz.vert"));

				// fragment shader
				shaderProgram_->RegisterShader(std::make_unique<nr::driver::Shader>(GL_FRAGMENT_SHADER, "fragmentShader", "fragmentShader.frag"));
				shaderProgram2_->RegisterShader(std::make_unique<nr::driver::Shader>(GL_FRAGMENT_SHADER, "fragmentShader", "fragmentShader.frag"));

			
			}
			bool InitProgram(const unsigned int& windowWidth, const unsigned int& windowHeight, const char* windowName) {
				if (!glfwInit() || !InitWindow(windowWidth, windowHeight, windowName) || !InitContext()) return false;
				InitCallbacks();
				InitArrays();
				InitShaders();
				shaderProgram_->Run();
				shaderProgram2_->Run();
				return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
			
			}
		}
		void Render() {
			shaderProgram_->Use();
			projectionMatrix_ = glm::mat4(1.0f);
			projectionMatrix_ = glm::perspective(glm::radians(45.0f), (float)1000 / (float)1000, 0.1f, 500.0f);
			shaderProgram_->SetUniformMat4("projectionMatrix", projectionMatrix_);
			shaderProgram_->SetUniformFloat("amplitude", 1);
			shaderProgram_->SetUniformFloat("particleCount", NUM_POINTS);

			shaderProgram2_->Use();
			shaderProgram2_->SetUniformMat4("projectionMatrix", projectionMatrix_);
			shaderProgram2_->SetUniformFloat("amplitude", 1);
			shaderProgram2_->SetUniformFloat("particleCount", NUM_POINTS);

			int frameNumber = 0;
			while (!glfwWindowShouldClose(window_)) {
				// clear the screen
				glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);



				glm::mat4 viewMatrix_ = glm::mat4(1.0f);
				viewMatrix_ = glm::lookAt(camera_->CameraPosition(), camera_->CameraPosition() + camera_->CameraFront(), camera_->CameraUp());

				shaderProgram_->Use();
				shaderProgram_->SetUniformMat4("viewMatrix", viewMatrix_);
				shaderProgram_->SetUniformFloat("frameNumber", frameNumber);

				glBindVertexArray(VAO_);

				glDrawArrays(GL_LINE_STRIP, 0, NUM_POINTS);

				shaderProgram2_->Use();
				shaderProgram2_->SetUniformMat4("viewMatrix", viewMatrix_);
				shaderProgram2_->SetUniformFloat("frameNumber", frameNumber);
				glDrawArrays(GL_LINE_STRIP, 0, NUM_POINTS);


				glfwPollEvents();
				glfwSwapBuffers(window_);
				++frameNumber;
			}
		}

	}

}