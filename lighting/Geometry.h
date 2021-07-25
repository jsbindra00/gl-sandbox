#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <vector>
#include <algorithm>


namespace nr {
	namespace geometry {
		template<typename VertexType>
		class Shape {
		protected:
	
		public:
			std::vector<float> vertices;
			std::vector<unsigned int> indices;
			Shape(const unsigned int& nPoints)
			:vertices(nPoints){
			}
			Shape() {
			}
		};
		class Cube : public nr::geometry::Shape<glm::vec3> {
		public:
			Cube(const glm::vec3& f1botLeft, const float& sideDim)
				{
				
				for (unsigned int i = 0; i < 2; ++i) {
					float faceOffset{ (i % 2) * sideDim };
					vertices.insert(vertices.end(), { f1botLeft.x,f1botLeft.y,f1botLeft.z + faceOffset });
					vertices.insert(vertices.end(), { f1botLeft.x + sideDim,f1botLeft.y,f1botLeft.z + faceOffset });
					vertices.insert(vertices.end(), { f1botLeft.x + sideDim,f1botLeft.y + sideDim,f1botLeft.z + faceOffset });
					vertices.insert(vertices.end(), { f1botLeft.x ,f1botLeft.y + sideDim,f1botLeft.z + faceOffset });
				}


				indices.insert(indices.end(),
					{
						//front
						0,1,2,
						0,3,2,
						// back
						4,5,6,
						4,7,6,
						// top
						3,2,6,
						3,7,6,
						// bottom
						0,1,5,
						0,4,5,
						// left
						4,0,3,
						4,7,3,
						// right
						1,2,6,
						1,5,6
					});
			
			}
			static float VertexCount() {
				return 8;
			}

		};
	}
}