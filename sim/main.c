#define GLT_IMPL
#include <ct/glt.h>
#include <ct/vt.h>

#include "pathing.h"
#include <cglm/struct.h>

#define SCR_WIDTH 1296
#define SCR_HEIGHT 1298
#define NUM_STUDENTS 12
//sample room D342, Length of 8.638 x8.654. meter to pixel of ratio of 1:150
//

enum
{
	STUDENT_MOVING,
	STUDENT_WAITING,
};

struct Student
{
	vec2s position;
	int cooldown;
	int mode;

	vtVec(vec2s) path;
};

vec2s classroomVerticies[] = {
	{0, 0},
	{791, 0},
	{791, 928},
	{0, 928},
};

void renderRoom(vec2s* verticies, int numVerticies, GLTvertexStore roomVs, GLuint polyShader)
{
	vec2s mesh[numVerticies + 1];
	memcpy(mesh, verticies, numVerticies * sizeof(vec2s));
	mesh[numVerticies] = verticies[0];
	gltVertexStoreSetData(roomVs, 0, sizeof(mesh), mesh, GL_DYNAMIC_DRAW);

	glUseProgram(polyShader);
	glUniform2f(glGetUniformLocation(polyShader, "screenSize"), SCR_WIDTH, SCR_HEIGHT);
	glUniform3f(glGetUniformLocation(polyShader, "wallColor"), 1, 1, 1);

	gltUseVertexStore(roomVs);

	glDrawArrays(GL_LINE_STRIP, 0, numVerticies + 1);
}

void renderTables(struct Table* tables, int numTables, GLTvertexStore tableVs, GLuint polyShader)
{
	for (int i = 0; i < numTables; i++)
	{
		vec2s mesh[vtLen(tables[i].verticies) + 1];
		memcpy(mesh, tables[i].verticies, vtLen(tables[i].verticies) * sizeof(vec2s));
		mesh[vtLen(tables[i].verticies)] = tables[i].verticies[0];
		gltVertexStoreSetData(tableVs, 0, sizeof(mesh), mesh, GL_DYNAMIC_DRAW);

		glUseProgram(polyShader);
		glUniform2f(glGetUniformLocation(polyShader, "screenSize"), SCR_WIDTH, SCR_HEIGHT);
		glUniform3f(glGetUniformLocation(polyShader, "wallColor"), 1, 0, 0);

		gltUseVertexStore(tableVs);

		glDrawArrays(GL_LINE_STRIP, 0, vtLen(tables[i].verticies) + 1);
	}
}

void renderStudents(
	struct Student* students, int numStudents, GLTvertexStore studentVs, GLuint studentShader)
{
	vec2s mesh[numStudents];

	for (int i = 0; i < numStudents; i++)
		mesh[i] = students[i].position;
	gltVertexStoreSetData(studentVs, 0, sizeof(mesh), mesh, GL_DYNAMIC_DRAW);

	glUseProgram(studentShader);
	glUniform2f(glGetUniformLocation(studentShader, "screenSize"), SCR_WIDTH, SCR_HEIGHT);
	gltUseVertexStore(studentVs);
	glDrawArrays(GL_POINTS, 0, numStudents);
}

// http://alienryderflex.com/polygon/
bool pointInPolygon(vec2s* verticies, int numVerticies, vec2s curr)
{
	int j = numVerticies - 1;
	bool oddNodes = false;

	for (int i = 0; i < numVerticies; i++)
	{
		if (verticies[i].y < curr.y && verticies[j].y >= curr.y ||
			verticies[j].y < curr.y && verticies[i].y >= curr.y)
		{
			if (verticies[i].x + (curr.y - verticies[i].y) /
						     (verticies[j].y - verticies[i].y) *
						     (verticies[j].x - verticies[i].x) <
				curr.x)
			{
				oddNodes = !oddNodes;
			}
		}
		j = i;
	}

	return oddNodes;
}

vec2s genNewTarget(vec2s* verticies, int numVerticies, struct Table* tables, int numTables)
{
	for (;;)
	{
		vec2s curr = {
			rand() / (float) RAND_MAX * SCR_WIDTH,
			rand() / (float) RAND_MAX * SCR_WIDTH,
		};

		if (pointInPolygon(verticies, numVerticies, curr))
		{
			bool inTable = false;
			for (int i = 0; i < numTables; i++)
				if (pointInPolygon(
					    tables[i].verticies, vtLen(tables[i].verticies), curr))
				{
					inTable = true;
					break;
				}

			if (!inTable)
			{
				return curr;
			}
		}
	}
}

vtVec(vec2s) createPath(vec2s pos, vec2s dest, vtVec(vtVec(struct DijkstraNode)) paths,
	vtVec(vec2s) verticies, vtVec(struct Table) tables, vtVec(vec2s) room)
{
	vtVec(int) posReachable = vtInit(int, 0);
	vtVec(int) destReachable = vtInit(int, 0);

	for (int i = 0; i < vtLen(verticies); i++)
	{
		if (pointIsReachable(pos, verticies[i], room, tables))
			vtPush(&posReachable, i);
		if (pointIsReachable(dest, verticies[i], room, tables))
			vtPush(&destReachable, i);
	}

	vtVec(vec2s) ret = vtInit(vec2s, 0);
	vtPush(&ret, dest);

	if (pointIsReachable(pos, dest, room, tables))
		return ret;

	int bestPosIndex = 0;
	int bestDestIndex = 0;
	float minCost = INFINITY;
	for (int i = 0; i < vtLen(posReachable); i++)
		for (int j = 0; j < vtLen(destReachable); j++)
		{
			float posToPathDist = glms_vec2_distance(pos, verticies[posReachable[i]]);
			float destToPathDist =
				glms_vec2_distance(dest, verticies[destReachable[j]]);
			float pathLen = paths[posReachable[i]][destReachable[j]].rootDistance;

			float totalLen = posToPathDist + destToPathDist + pathLen;

			if (minCost > totalLen)
			{
				minCost = totalLen;
				bestPosIndex = i;
				bestDestIndex = j;
			}
		}

	int currIndex = destReachable[bestDestIndex];
	while (currIndex != -1)
	{
		vtPush(&ret, verticies[currIndex]);
		currIndex = paths[posReachable[bestPosIndex]][currIndex].prevNode;
	}

	return ret;
}

void updateStudent(vtVec(struct Student) students, vtVec(struct Table) tables, vtVec(vec2s) room,
	vtVec(vtVec(struct DijkstraNode)) paths, vtVec(vec2s) verticies)
{
	int numStudents = vtLen(students);
	int numTables = vtLen(tables);
	int numClassroomVerticies = vtLen(room);

	for (int i = 0; i < numStudents; i++)
	{
		if (students[i].mode == STUDENT_WAITING)
		{
			students[i].cooldown -= 1;
			if (students[i].cooldown == 0)
			{
				vec2s dest = genNewTarget(
					room, numClassroomVerticies, tables, numTables);

				students[i].mode = STUDENT_MOVING;
				students[i].path = createPath(
					students[i].position, dest, paths, verticies, tables, room);
			}
		}
		else if (students[i].mode == STUDENT_MOVING)
		{
			vec2s target = students[i].path[vtLen(students[i].path) - 1];

			if (glms_vec2_distance(students[i].position, target) < 10)
			{
				if (vtLen(students[i].path) == 1)
				{
					vtFree(students[i].path);

					students[i].mode = STUDENT_WAITING;
					students[i].cooldown = rand() / (float) RAND_MAX * 100 + 50;
				}

				vtPop(students[i].path);
			}
			else
			{
				vec2s direction = glms_vec2_sub(target, students[i].position);
				students[i].position = glms_vec2_add(students[i].position,
					glms_vec2_scale_as(
						direction, rand() / (float) RAND_MAX * 5));

				students[i].position.x += rand() / (float) RAND_MAX * 2 - 1;
				students[i].position.y += rand() / (float) RAND_MAX * 2 - 1;
			}
		}
	}
}

int main()
{
	GLFWwindow* window = gltCreateDefaultContext(SCR_WIDTH, SCR_HEIGHT, "", NULL);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

	GLuint polyShader = gltCreateShader("poly.vert", "poly.frag");
	GLTvertexStore roomVs = gltCreateVertexStore(1);
	gltUseVertexStoreBuffer(roomVs, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, NULL);

	GLTvertexStore tableVs = gltCreateVertexStore(1);
	gltUseVertexStoreBuffer(tableVs, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, NULL);

	GLuint studentShader = gltCreateShader("student.vert", "student.frag");
	GLTvertexStore studentVs = gltCreateVertexStore(1);
	gltUseVertexStoreBuffer(studentVs, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, NULL);

	vtVec(vec2s) classroom = vtInit(vec2s, 0);
	vtPushArr(&classroom, classroomVerticies, 4);

	struct Student student = {
		.position = {200, 200},
		.mode = STUDENT_WAITING,
		.cooldown = 1,
	};

	vtVec(struct Student) students = vtInit(struct Student, NUM_STUDENTS);
	for (int i = 0; i < NUM_STUDENTS; i++)
		students[i] = student;

	vtVec(struct Table) tables = vtInit(struct Table, 2);
	tables[0].verticies = vtInit(vec2s, 4);
	tables[0].verticies[0] = (vec2s){155, 150};
	tables[0].verticies[1] = (vec2s){155, 350};
	tables[0].verticies[2] = (vec2s){205, 350};
	tables[0].verticies[3] = (vec2s){205, 150};
	tables[1].verticies = vtInit(vec2s, 4);
	tables[1].verticies[0] = (vec2s){275, 150};
	tables[1].verticies[1] = (vec2s){275, 350};
	tables[1].verticies[2] = (vec2s){325, 350};
	tables[1].verticies[3] = (vec2s){325, 150};

	vtVec(vec2s) vertexList;
	vtVec(vtVec(struct Edge)) graph = genGraph(classroom, tables, &vertexList);
	vtVec(vtVec(struct DijkstraNode)) paths = dijkstra(graph);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		glClear(GL_COLOR_BUFFER_BIT);
		renderRoom(classroom, vtLen(classroom), roomVs, polyShader);
		renderTables(tables, vtLen(tables), tableVs, polyShader);
		renderStudents(students, NUM_STUDENTS, studentVs, studentShader);
		updateStudent(students, tables, classroom, paths, vertexList);

		glfwSwapBuffers(window);
	}

	vtFree(students);
	vtFree(tables);
	vtFree(vertexList);

	for (int i = 0; i < vtLen(graph); i++)
		vtFree(graph[i]);
	vtFree(graph);

	for (int i = 0; i < vtLen(paths); i++)
		vtFree(paths[i]);
	vtFree(paths);
}
