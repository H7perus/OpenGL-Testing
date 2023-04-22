// Helper class; draws the world as seen by Bullet.
// This is very handy to see it Bullet's world matches yours
// How to use this class :
// Declare an instance of the class :
// 
// dynamicsWorld->setDebugDrawer(&mydebugdrawer);
// Each frame, call it :
// mydebugdrawer.SetMatrices(ViewMatrix, ProjectionMatrix);
// dynamicsWorld->debugDrawWorld();

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include "shader.h"


#include "Bullet3.24/btBulletDynamicsCommon.h"
#include "Bullet3.24/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include "Bullet3.24/BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"
#include "Bullet3.24/BulletCollision/CollisionShapes/btTriangleShape.h"



GLuint debugVBO, debugVAO;
GLuint debugLineSize;
GLfloat debugPoints[12];
class BulletDebugDrawer_OpenGL : public btIDebugDraw {
public:
	BulletDebugDrawer_OpenGL() {
		lineShader = Shader("physicsdebug.vert", "physicsdebug.frag");
		m = 0;
	}

	
	void SetMatrices(glm::mat4 pViewMatrix, glm::mat4 pProjectionMatrix)
	{
		lineShader.setMat4("projection", pProjectionMatrix);
		lineShader.setMat4("view", pViewMatrix);
	}
	virtual void drawAll() 
	{

		glBindVertexArray(debugVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));


		glDrawArrays(GL_LINES, 0, debugLineSize);
		glBindVertexArray(0);

		glDeleteBuffers(1, &debugVBO);
		glDeleteVertexArrays(1, &debugVAO);
		glGenVertexArrays(1, &debugVAO);
		glGenBuffers(1, &debugVBO);
		glBindBuffer(GL_ARRAY_BUFFER, debugVBO);
		glBufferData(GL_ARRAY_BUFFER, debugLineSize * 4, 0, GL_DYNAMIC_DRAW);
		debugLineSize = 0;
	};

	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
	{
		// Vertex data
		debugPoints[0] = from.x();
		debugPoints[1] = from.y();
		debugPoints[2] = from.z();
		debugPoints[3] = color.x();
		debugPoints[4] = color.y();
		debugPoints[5] = color.z();
		//if (color.y() > 0.5)
		//{
			//return;
		//}

		debugPoints[6] = to.x();
		debugPoints[7] = to.y();
		debugPoints[8] = to.z();
		debugPoints[9] = color.x();
		debugPoints[10] = color.y();
		debugPoints[11] = color.z();
		//glBindBuffer(GL_ARRAY_BUFFER, debugVBO);
		glBufferSubData(GL_ARRAY_BUFFER, debugLineSize * sizeof(GLfloat), sizeof(debugPoints), debugPoints);
		debugLineSize += 12;
		//std::cout << debugLineSize << std::endl;
	}
	virtual void drawContactPoint(const btVector3&, const btVector3&, btScalar, int, const btVector3&) {}
	virtual void reportErrorWarning(const char*) {}
	virtual void draw3dText(const btVector3&, const char*) {}
	virtual void setDebugMode(int p) {
		m = p;
	}
	int getDebugMode(void) const { return 3; }
	int m;
	Shader lineShader;
};